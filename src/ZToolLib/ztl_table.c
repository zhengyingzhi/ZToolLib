#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "ztl_atomic.h"
#include "ztl_hash.h"
#include "ztl_mem.h"
#include "ztl_palloc.h"
#include "ztl_table.h"
#include "ztl_times.h"
#include "ztl_threads.h"


typedef struct table_intl_st {
    unsigned long   size, sizemask, used;
    table_node_t*   buckets;
} table_intl_t;

struct table_st
{
    table_intl_t    ti[2];
    int64_t         rehashidx;
    int             iterators;
    int             can_resize;
    cmp_pt          cmp;
    hash_pt         hash;
    free_pt         kfree;
    free_pt         vfree;
    table_iter_t    iter;
    ztl_pool_t*     pool;
    table_node_t    idles;
    unsigned long   reserve_size;
    ztl_thread_mutex_t  lock;
    ztl_thread_rwlock_t rwlock;
};

struct table_node_st {
    table_node_t    next;
    const void*     key;
    int             keysz;
    union {
        void*   value;
        int     i;
        float   f;
        double  d;
    }v;
};

struct table_iter_st {
    table_t         table;
    int32_t         i;
    int32_t         index;
    int32_t         safe;
    table_node_t    curr;
    table_node_t    next;
};

static unsigned int table_force_resize_ratio = 5;

static void table_reset(table_intl_t* tip)
{
    tip->size = tip->sizemask = tip->used = 0;
    tip->buckets = NULL;
}

static unsigned long table_next_power(unsigned long size)
{
    unsigned long i = ZTL_TABLE_INIT_SIZE;

    if (size > LONG_MAX)
        return LONG_MAX;
    for (;;)
    {
        if (i >= size)
            return i;
        i *= 2;
    }
}

static int table_expand_if_needed(table_t table)
{
    if (table->rehashidx != -1)
        return 0;
    if (table->ti[0].size == 0)
        return table_expand(table, table->reserve_size);
    if (table->ti[0].used >= table->ti[0].size &&
        (table->can_resize || table->ti[0].used / table->ti[0].size > table_force_resize_ratio))
        return table_expand(table, table->ti[0].used << 1);
    return 0;
}

static void table_rehash_step(table_t table)
{
    if (table->iterators == 0)
        table_rehash(table, 1);
}

static table_node_t table_node_alloc(table_t table)
{
    table_node_t node;
    if (table->idles)
    {
        node = table->idles;
        table->idles = table->idles->next;
    }
    else
    {
        node = ztl_palloc(table->pool, sizeof(struct table_node_st));
    }
    return node;
}

static void table_node_free(table_t table, table_node_t node)
{
    if (table->idles)
        node->next = table->idles;
    table->idles = node;
}

static long long get_ms(void)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return ((long long)tv.tv_sec) * 1000 + tv.tv_usec / 1000;
}

//////////////////////////////////////////////////////////////////////////
table_t table_new(cmp_pt cmp, hash_pt hash, free_pt kfree, free_pt vfree)
{
    table_t table;
    ztl_pool_t* pool;
    ztl_thread_mutexattr_t mattr;

    pool = ztl_create_pool(4096);
    if (!pool) {
        return NULL;
    }
    table = (table_t)ztl_pcalloc(pool, sizeof(struct table_st));
    table->pool = pool;

    table_reset(&table->ti[0]);
    table_reset(&table->ti[1]);
    table->rehashidx    = -1;
    table->iterators    = 0;
    table->can_resize   = 1;
    table->cmp          = cmp;
    table->hash         = hash;
    table->kfree        = kfree;
    table->vfree        = vfree;
    table->iter         = (table_iter_t)ALLOC(sizeof(struct table_iter_st));
    table->idles        = NULL;
    table->reserve_size = ZTL_TABLE_INIT_SIZE;

    ztl_thread_mutexattr_init(&mattr);
    ztl_thread_mutexattr_settype(&mattr, ZTL_THREAD_MUTEX_RECURSIVE_NP);
    ztl_thread_mutex_init(&table->lock, &mattr);
    ztl_thread_mutexattr_destroy(&mattr);
    ztl_thread_rwlock_init(&table->rwlock);

    return table;
}

void table_free(table_t* tp)
{
    if (tp == NULL || *tp == NULL)
        return;

    ztl_pool_t* pool = (*tp)->pool;
    ztl_thread_mutex_destroy(&(*tp)->lock);
    ztl_thread_rwlock_destroy(&(*tp)->rwlock);
    table_clear(*tp);
    ztl_destroy_pool(pool);
    *tp = NULL;
}

int table_size(table_t table)
{
    return table ? (int)(table->ti[0].size + table->ti[1].size) : 0;
}

int table_length(table_t table)
{
    return table ? (int)(table->ti[0].used + table->ti[1].used) : 0;
}

const void* table_node_key(table_node_t node, int* pkeysz)
{
    if (node == NULL)
        return NULL;
    if (pkeysz)
        *pkeysz = node->keysz;
    return node->key;
}

void* table_node_value(table_node_t node)
{
    if (node == NULL)
        return NULL;
    return node->v.value;
}

int table_node_int(table_node_t node)
{
    if (node == NULL)
        return 0;
    return node->v.i;
}

float table_node_float(table_node_t node)
{
    if (node == NULL)
        return 0;
    return node->v.f;
}

double table_node_double(table_node_t node)
{
    if (node == NULL)
        return 0;
    return node->v.d;
}

table_iter_t table_iter_new(table_t table)
{
    table_iter_t iter;
    iter = table->iter;
    if (!atomic_casptr(&table->iter, iter, NULL) || !iter)
    {
        iter = (table_iter_t)malloc(sizeof(struct table_iter_st));
    }

    iter->table = table;
    iter->i = 0;
    iter->index = -1;
    iter->safe = 0;
    iter->curr = NULL;
    iter->next = NULL;
    return iter;
}

table_iter_t table_iter_safe_new(table_t table)
{
    table_iter_t iter = table_iter_new(table);

    if (iter)
        iter->safe = 1;
    return iter;
}

void table_iter_rewind(table_iter_t iter)
{
    if (iter == NULL)
        return;
    iter->i = 0;
    iter->index = -1;
}

table_node_t table_next(table_iter_t iter)
{
    for (;;)
    {
        if (iter->curr == NULL)
        {
            table_intl_t* tip;
            tip = &iter->table->ti[iter->i];

            if (iter->safe && iter->i == 0 && iter->index == -1)
                ++iter->table->iterators;
            ++iter->index;
            if (iter->index >= (int32_t)tip->size)
            {
                if (iter->table->rehashidx != -1 && iter->i == 0)
                {
                    ++iter->i;
                    iter->index = 0;
                    tip = &iter->table->ti[iter->i];
                }
                else
                {
                    break;
                }
            }
            iter->curr = tip->buckets[iter->index];
        }
        else
        {
            iter->curr = iter->next;
        }

        if (iter->curr)
        {
            iter->next = iter->curr->next;
            return iter->curr;
        }
    }
    return NULL;
}

void table_iter_free(table_iter_t *tip)
{
    table_iter_t iter;
    if (tip == NULL || *tip == NULL)
        return;

    iter = *tip;

    if (iter->safe && !(iter->i == 0 && iter->index == -1))
        --iter->table->iterators;

    if (!atomic_casptr(&iter->table->iter, NULL, iter))
    {
        FREE(iter);
    }
    *tip = NULL;
}

int table_expand(table_t table, unsigned long size)
{
    unsigned long n = table_next_power(size);
    table_intl_t ti;

    if (table == NULL || table->rehashidx != -1 || table->ti[0].used > size)
        return -1;
    ti.size = n;
    ti.sizemask = n - 1;
    ti.used = 0;
    if ((ti.buckets = CALLOC(1, n * sizeof(table_node_t))) == NULL)
        return -1;

    table->reserve_size = n;
    if (table->ti[0].buckets == NULL)
    {
        table->ti[0] = ti;
    }
    else
    {
        table->ti[1] = ti;
        table->rehashidx = 0;
    }
    return 0;
}

int table_resize(table_t table)
{
    unsigned long size;

    if (table == NULL || !table->can_resize || table->rehashidx != -1)
        return -1;
    if ((size = table->ti[0].used) < ZTL_TABLE_INIT_SIZE)
        size = ZTL_TABLE_INIT_SIZE;
    return table_expand(table, size);
}

inline void table_resize_enable(table_t table)
{
    table->can_resize = 1;
}

inline void table_resize_disable(table_t table)
{
    table->can_resize = 0;
}

table_node_t table_insert_raw(table_t table, const void* key, int keysz)
{
    table_node_t  node;
    table_intl_t* tip;
    unsigned long index;

    if (table == NULL)
        return NULL;
    if ((node = table_find(table, key, keysz)))
        return node;
    if (table_expand_if_needed(table) == -1)
        return NULL;

    // if (NEW0(node) == NULL)
    if ((node = table_node_alloc(table)) == NULL)
        return NULL;
    tip = table->rehashidx != -1 ? &table->ti[1] : &table->ti[0];
    index = table->hash(key, keysz) & tip->sizemask;
    node->next = tip->buckets[index];
    node->key = key;
    node->keysz = keysz;
    tip->buckets[index] = node;
    ++tip->used;
    return node;
}

void table_set_value(table_node_t node, void* value)
{
    if (node == NULL)
        return;
    node->v.value = value;
}

void table_set_int(table_node_t node, int i)
{
    if (node == NULL)
        return;
    node->v.i = i;
}

void table_set_float(table_node_t node, float f)
{
    if (node == NULL)
        return;
    node->v.f = f;
}

void table_set_double(table_node_t node, double d)
{
    if (node == NULL)
        return;
    node->v.d = d;
}

void* table_insert(table_t table, const void* key, int keysz, void* value)
{
    table_node_t node;
    void* prev;

    if ((node = table_insert_raw(table, key, keysz)) == NULL)
        return NULL;
    prev = node->v.value ? node->v.value : NULL;
    node->v.value = value;
    return prev;
}

table_node_t table_find(table_t table, const void* key, int keysz)
{
    uint32_t i;
    uint64_t h;

    if (table == NULL || table->ti[0].buckets == NULL)
        return NULL;
    if (table->rehashidx != -1)
        table_rehash_step(table);
    h = table->hash(key, keysz);
    for (i = 0; i < 2; ++i)
    {
        uint32_t index = h & table->ti[i].sizemask;
        table_node_t node = table->ti[i].buckets[index];

        for (; node; node = node->next)
            if (table->cmp(node->key, key) == 0)
                return node;
        if (table->rehashidx == -1)
            return NULL;
    }
    return NULL;
}

void* table_get_value(table_t table, const void* key, int keysz)
{
    table_node_t node = table_find(table, key, keysz);

    return node ? node->v.value : NULL;
}

void* table_remove(table_t table, const void* key, int keysz)
{
    uint32_t i;
    uint64_t h;

    if (table == NULL || table->ti[0].buckets == NULL)
        return NULL;
    if (table->rehashidx != -1)
        table_rehash_step(table);
    h = table->hash(key, keysz);
    for (i = 0; i < 2; ++i)
    {
        uint32_t index = h & table->ti[i].sizemask;
        table_node_t curr = table->ti[i].buckets[index], prev = NULL;

        for (; curr; prev = curr, curr = curr->next)
        {
            if (table->cmp(curr->key, key) == 0)
            {
                void* value;

                if (prev == NULL)
                    table->ti[i].buckets[index] = curr->next;
                else
                    prev->next = curr->next;
                if (table->kfree)
                    table->kfree((void*)curr->key);
                value = curr->v.value;
                // FREE(curr);
                table_node_free(table, curr);
                --table->ti[i].used;
                return value;
            }
        }
        if (table->rehashidx == -1)
            break;
    }
    return NULL;
}

void table_clear(table_t table)
{
    unsigned long i, j;

    if (table == NULL || table->ti[0].buckets == NULL)
        return;
    for (i = 0; i < 2; ++i)
    {
        for (j = 0; j < table->ti[i].size && table->ti[i].used > 0; ++j)
        {
            table_node_t curr = table->ti[i].buckets[j], next;

            if (curr == NULL)
                continue;
            while (curr)
            {
                next = curr->next;
                if (table->kfree)
                    table->kfree((void*)curr->key);
                if (table->vfree)
                    table->vfree(curr->v.value);
                // FREE(curr);
                table_node_free(table, curr);
                --table->ti[i].used;
                curr = next;
            }
        }
        FREE(table->ti[i].buckets);
        table_reset(&table->ti[i]);
    }
    table->rehashidx = -1;
    table->iterators = 0;
}

/* Return 1 if there are still keys to move, otherwise 0 is returned. */
int table_rehash(table_t table, int n)
{
    if (table == NULL || table->rehashidx == -1)
        return 0;
    while (n--)
    {
        table_node_t curr, next;

        if (table->ti[0].used == 0)
        {
            FREE(table->ti[0].buckets);
            table->ti[0] = table->ti[1];
            table_reset(&table->ti[1]);
            table->rehashidx = -1;
            return 0;
        }
        /* FIXME */
        while (table->ti[0].buckets[table->rehashidx] == NULL)
            ++table->rehashidx;
        curr = table->ti[0].buckets[table->rehashidx];
        while (curr)
        {
            unsigned index = table->hash(curr->key, curr->keysz) & table->ti[1].sizemask;

            next = curr->next;
            curr->next = table->ti[1].buckets[index];
            table->ti[1].buckets[index] = curr;
            --table->ti[0].used;
            ++table->ti[1].used;
            curr = next;
        }
        table->ti[0].buckets[table->rehashidx] = NULL;
        ++table->rehashidx;
    }
    return 1;
}

void table_rehash_ms(table_t table, int ms)
{
    long long start = get_ms();

    while (table_rehash(table, 100))
        if (get_ms() - start > ms)
            break;
}

void table_lock(table_t table)
{
    if (table == NULL)
        return;
    ztl_thread_mutex_lock(&table->lock);
}

void table_unlock(table_t table)
{
    if (table == NULL)
        return;
    ztl_thread_mutex_unlock(&table->lock);
}

void table_rwlock_rdlock(table_t table)
{
    if (table == NULL)
        return;
    ztl_thread_rwlock_rdlock(&table->rwlock);
}

void table_rwlock_wrlock(table_t table)
{
    if (table == NULL)
        return;
    ztl_thread_rwlock_wrlock(&table->rwlock);
}

void table_rwlock_unlock(table_t table)
{
    if (table == NULL)
        return;
    ztl_thread_rwlock_unlock(&table->rwlock);
}

//////////////////////////////////////////////////////////////////////////
void table_default_kvfree(void* p)
{
    if (p)
        free(p);
}

int table_default_cmpstr(const void *x, const void *y)
{
    return strcmp((char *)x, (char *)y);
}

uint64_t table_default_hashstr(const void* val, int size)
{
    if (size <= 0)
        size = (int)strlen((char*)val);
    return ztl_murmur_hash2((unsigned char*)val, size);
}

int table_default_cmpint(const void* x, const void* y)
{
    return x == y ? 0 : -1;
}

uint64_t table_default_hashint(const void* val, int size)
{
    (void)size;
    return (uint64_t)val;
}
