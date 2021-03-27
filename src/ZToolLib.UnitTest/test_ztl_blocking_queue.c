#include <stdio.h>
#include <stdlib.h>
#include <ZToolLib/ztl_unit_test.h>
#include <ZToolLib/ztl_utils.h>
#include <ZToolLib/ztl_blocking_queue.h>


void Test_ztl_blocking_queue(ZuTest* zt)
{
    ztl_blocking_queue_t* zbq;

    int* pi;
    int* pr;
    zbq = ztl_bq_create(8, sizeof(void*));
    ztl_new_val(pi, int, 32);
    ztl_bq_push(zbq, &pi);
    ztl_bq_pop(zbq, &pr, 1000);
    ZuAssertPtrEquals(zt, pi, pr);
    ztl_bq_release(zbq);

    int i;
    int r;
    zbq = ztl_bq_create(8, sizeof(int));
    i = 32;
    ztl_bq_push(zbq, &i);
    ztl_bq_pop(zbq, &r, 1000);
    ZuAssertIntEquals(zt, i, r);
    ztl_bq_release(zbq);

    typedef struct
    {
        void*   data;
        int64_t type;
    }ztl_bq_data_t;
    ztl_bq_data_t di;
    ztl_bq_data_t dr;
    zbq = ztl_bq_create(8, sizeof(ztl_bq_data_t));
    ztl_new_val(di.data, int, 1);
    di.type = 2;
    ztl_bq_push(zbq, &di);
    ztl_bq_pop(zbq, &dr, 1000);
    ZuAssertInt64Equals(zt, di.type, dr.type);
    ZuAssertPtrEquals(zt, di.data, dr.data);
    ztl_bq_release(zbq);

    ztl_bq_data_t* pdi;
    ztl_bq_data_t* pdr;
    zbq = ztl_bq_create(8, sizeof(void*));
    pdi = (ztl_bq_data_t*)malloc(sizeof(ztl_bq_data_t));
    pdi->data = NULL;
    pdi->type = 3;
    ztl_bq_push(zbq, &pdi);
    ztl_bq_pop(zbq, &pdr, 1000);
    ZuAssertPtrEquals(zt, pdi, pdr);
    ztl_bq_release(zbq);
}
