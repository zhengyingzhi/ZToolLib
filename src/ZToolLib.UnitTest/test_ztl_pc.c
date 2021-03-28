#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ZToolLib/ztl_unit_test.h>

#include <ZToolLib/ztl_producer_consumer.h>
#include <ZToolLib/ztl_threads.h>
#include <ZToolLib/ztl_times.h>

static uint32_t gCounter = 0;
static uint64_t gSum = 0;

static bool _Test_pc_consumer_entry(ztl_producer_consumer_t* zpc, int64_t dtype, void* arg);

void Test_ztl_producer_consumer(ZuTest* zt)
{
    ztl_producer_consumer_t* zpc;

    zpc = ztl_pc_create(1024);
    ztl_pc_start(zpc);
    ztl_sleepms(10);

    uint32_t lCounter = 0;
    uint64_t lSum = 0;

    for (int i = 0; i < 100000; ++i)
    {
        int* pi = (int*)malloc(sizeof(int));
        *pi = i;

        while (0 != ztl_pc_post(zpc, _Test_pc_consumer_entry, i, pi))
            sleepms(0);

        lCounter++;
        lSum += i;
    }

    sleepms(100);

    ZuAssertTrue(zt, lCounter == gCounter);
    ZuAssertTrue(zt, lSum == gSum);
}


static bool _Test_pc_consumer_entry(ztl_producer_consumer_t* zpc, int64_t dtype, void* arg)
{
    (void)zpc;
    (void)dtype;

    int* pi = (int*)arg;
    gCounter++;
    gSum += *pi;

    if ((*pi & 15) == 0) {
        sleepms(0);
    }

    free(pi);

    return true;
}
