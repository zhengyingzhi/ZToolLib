#include <stdio.h>
#include <ZToolLib/ztl_times.h>

#include "high_time.h"

#ifdef _MSC_VER
#include <Windows.h>
#endif


int high_time_init(high_time_t* ht, int speed)
{
    ht->prev_tv = 0;
    ht->prev_time = 0;
    ht->speed = speed;
    ht->sleep_err = 0;
    return 0;
}

void high_time_update_first(high_time_t* ht, int32_t curr_time)
{
    ht->prev_time = curr_time;
    ht->prev_tv = get_timestamp_us();
}

int high_time_update_and_sleep(high_time_t* ht, int32_t curr_time)
{
    int32_t elapsed_us, delta_time, sleep_time;
    int64_t now_tv;

    if (curr_time == ht->prev_time) {
        return 0;
    }

    now_tv = get_timestamp_us();
    elapsed_us = (int32_t)(now_tv - ht->prev_tv);

    delta_time = curr_time - ht->prev_time;
    ht->prev_time = curr_time;

    // TODO:中午休市时，时间差如何处理？

    sleep_time = delta_time * 1000 - elapsed_us;
    if (sleep_time > 0)
    {
        fprintf(stderr, ">>>>>>dbg sleep_time=%d curr_time=%d ...., cost=%d\n", sleep_time, curr_time, ht->sleep_err);
#ifdef _MSC_VER
        // delta = delta / ht->speed;
        // ztl_sleepus(delta / ht->speed);
        sleep_time = (sleep_time - ht->sleep_err) / 1000 / ht->speed;
        ztl_sleepms(sleep_time);
        sleep_time *= 1000; // for calc sleep_cost since here is sleepms
#else
        // ztl_sleepus(delta / ht->speed);
        sleep_time = (sleep_time - ht->sleep_err) / ht->speed;
        ztl_sleepus(sleep_time);
#endif//_MSC_VER
        fprintf(stderr, ">>>>>>dbg sleep_time=%d curr_time=%d done\n", sleep_time, curr_time);

        ht->prev_tv = get_timestamp_us();

        // FIXME: calc the sleep cost?
        ht->sleep_err = (int32_t)(ht->prev_tv - now_tv) - sleep_time;
    }
    // else {
    //     ht->tv = now_tv;
    // }

    return 0;
}
