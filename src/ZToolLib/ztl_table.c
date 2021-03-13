#include <stdlib.h>
#include <string.h>
#include <limits.h>

#if _MSC_VER
#include <WinSock2.h>
#define HAVE_PTHREAD    0
#else
#include <pthread.h>
#define HAVE_PTHREAD    1
#endif//_MSC_VER

#include "ztl_mem.h"
#include "ztl_table.h"
#include "ztl_times.h"


typedef struct table_intl_t {
    unsigned long   size, sizemask, used;
    table_node_t*   buckets;
} table_intl_t;

struct table_st {
    table_intl_t    ti[2];
    int64_t         rehashidx;
    int             iterators;
    cmp_ptr         cmp;
    hash_ptr        hash;
    free_ptr        kfree;
    free_ptr        vfree;
#if HAVE_PTHREAD
    pthread_mutex_t     lock;
    pthread_rwlock_t    rwlock;
#else
    CRITICAL_SECTION    lock;
    CRITICAL_SECTION    rwlock;
#endif//HAVE_PTHREAD
};

struct table_node_st {
    table_node_t    next;
    const void*     key;
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
    uint32_t        index;
    int32_t         safe;
    table_node_t    curr;
    table_node_t    next;
};

static int          table_can_resize = 1;
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
        return table_expand(table, ZTL_TABLE_INIT_SIZE);
    if (table->ti[0].used >= table->ti[0].size &&
        (table_can_resize || table->ti[0].used / table->ti[0].size > table_force_resize_ratio))
        return table_expand(table, table->ti[0].used << 1);
    return 0;
}

static void table_rehash_step(table_t table)
{
    if (table->iterators == 0)
        table_rehash(table, 1);
}

static long long get_ms(void)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return ((long long)tv.tv_sec) * 1000 + tv.tv_usec / 1000;
}

table_t table_new(cmp_ptr cmp, hash_ptr hash, free_ptr kfree, free_ptr vfree)
{
    table_t table;
#if HAVE_PTHREAD
    pthread_mutexattr_t mattr;
    pthread_rwlockattr_t rwattr;
#endif//HAVE_PTHREAD

    if (NEW(table) == NULL)
        return NULL;
    table_reset(&table->ti[0]);
    table_reset(&table->ti[1]);
    table->rehashidx    = -1;
    table->iterators    = 0;
    table->cmp          = cmp;
    table->hash         = hash;
    table->kfree        = kfree;
    table->vfree        = vfree;

#if HAVE_PTHREAD
    pthread_mutexattr_init(&mattr);
    pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_ADAPTIVE_NP);
    pthread_mutex_init(&table->lock, &mattr);
    pthread_mutexattr_destroy(&mattr);
    pthread_rwlockattr_init(&rwattr);
    pthread_rwlockattr_setkind_np(&rwattr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
    pthread_rwlock_init(&table->rwlock, &rwattr);
    pthread_rwlockattr_destroy(&rwattr);
#else
    InitializeCriticalSection(&table->lock);
    InitializeCriticalSection(&table->rwlock);
#endif//HAVE_PTHREAD
    return table;
}

void table_free(table_t *tp)
{
    if (tp == NULL || *tp == NULL)
        return;

#if HAVE_PTHREAD
    pthread_mutex_destroy(&(*tp)->lock);
    pthread_rwlock_destroy(&(*tp)->rwlock);
#else
    DeleteCriticalSection(&(*tp)->lock);
    DeleteCriticalSection(&(*tp)->rwlock);
#endif//HAVE_PTHREAD
    table_clear(*tp);
    FREE(*tp);
}

int table_size(table_t table)
{
    if (table == NULL)
        return 0;
    return (int)(table->ti[0].size + table->ti[1].size);
}

int table_length(table_t table)
{
    if (table == NULL)
        return 0;
    return (int)(table->ti[0].used + table->ti[1].used);
}

const void* table_node_key(table_node_t node)
{
    if (node == NULL)
        return NULL;
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

    if (table == NULL || NEW0(iter) == NULL)
        return NULL;
    iter->table = table;
    iter->i = 0;
    iter->index = -1;
    iter->safe = 0;
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
            table_intl_t *tip = &iter->table->ti[iter->i];

            if (iter->safe && iter->i == 0 && iter->index == -1)
                ++iter->table->iterators;
            ++iter->index;
            if (iter->index >= tip->size)
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
    if (tip == NULL || *tip == NULL)
        return;
    if ((*tip)->safe && !((*tip)->i == 0 && (int)(*tip)->index == -1))
        --(*tip)->table->iterators;
    FREE(*tip);
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

    if (table == NULL || !table_can_resize || table->rehashidx != -1)
        return -1;
    if ((size = table->ti[0].used) < ZTL_TABLE_INIT_SIZE)
        size = ZTL_TABLE_INIT_SIZE;
    return table_expand(table, size);
}

inline void table_resize_enable(void)
{
    table_can_resize = 1;
}

inline void table_resize_disable(void)
{
    table_can_resize = 0;
}

table_node_t table_insert_raw(table_t table, const void* key)
{
    table_node_t  node;
    table_intl_t* tip;
    unsigned long index;

    if (table == NULL)
        return NULL;
    if ((node = table_find(table, key)))
        return node;
    if (table_expand_if_needed(table) == -1)
        return NULL;
    if (NEW0(node) == NULL)
        return NULL;
    tip = table->rehashidx != -1 ? &table->ti[1] : &table->ti[0];
    index = table->hash(key) & tip->sizemask;
    node->next = tip->buckets[index];
    node->key = key;
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

void* table_insert(table_t table, const void* key, void* value)
{
    table_node_t node;
    void* prev;

    if (key == NULL)
        return NULL;
    if ((node = table_insert_raw(table, key)) == NULL)
        return NULL;
    prev = node->v.value ? node->v.value : NULL;
    node->v.value = value;
    return prev;
}

table_node_t table_find(table_t table, const void* key)
{
    uint32_t i;
    uint64_t h;

    if (table == NULL || table->ti[0].buckets == NULL)
        return NULL;
    if (table->rehashidx != -1)
        table_rehash_step(table);
    h = table->hash(key);
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

void* table_get_value(table_t table, const void* key)
{
    table_node_t node = table_find(table, key);

    return node ? node->v.value : NULL;
}

void* table_remove(table_t table, const void* key)
{
    uint32_t i;
    uint64_t h;

    if (table == NULL || table->ti[0].buckets == NULL)
        return NULL;
    if (table->rehashidx != -1)
        table_rehash_step(table);
    h = table->hash(key);
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
                    table->kfree(curr->key);
                value = curr->v.value;
                FREE(curr);
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
                    table->kfree(curr->key);
                if (table->vfree)
                    table->vfree(curr->v.value);
                FREE(curr);
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
            unsigned index = table->hash(curr->key) & table->ti[1].sizemask;

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
#if HAVE_PTHREAD
    pthread_mutex_lock(&table->lock);
#else
    EnterCriticalSection(&table->lock);
#endif
}

void table_unlock(table_t table)
{
    if (table == NULL)
        return;
#if HAVE_PTHREAD
    pthread_mutex_unlock(&table->lock);
#else
    LeaveCriticalSection(&table->lock);
#endif
}

void table_rwlock_rdlock(table_t table)
{
    if (table == NULL)
        return;
#if HAVE_PTHREAD
    pthread_rwlock_rdlock(&table->rwlock);
#else
    EnterCriticalSection(&table->lock);
#endif
}

void table_rwlock_wrlock(table_t table)
{
    if (table == NULL)
        return;
#if HAVE_PTHREAD
    pthread_rwlock_wrlock(&table->rwlock);
#else
    EnterCriticalSection(&table->lock);
#endif
}

void table_rwlock_unlock(table_t table)
{
    if (table == NULL)
        return;
#if HAVE_PTHREAD
    pthread_rwlock_unlock(&table->rwlock);
#else
    LeaveCriticalSection(&table->lock);
#endif
}

