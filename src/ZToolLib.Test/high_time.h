#ifndef _ZTL_HIGH_TIME_H_
#define _ZTL_HIGH_TIME_H_

#include <stdint.h>

typedef struct high_time_st high_time_t;
struct high_time_st
{
    int64_t     prev_tv;
    int32_t     prev_time;
    int32_t     speed;
    int32_t     sleep_err;
};


/* 初始化 */
int high_time_init(high_time_t* ht, int speed);

/* 一般在第一次使用时（业务开始前）调用 */
void high_time_update_first(high_time_t* ht, int32_t curr_time);

/* 取下一条行情，然后自动sleep对应的时间,
 * @param curr_time 当前行情时间戳，毫秒，如 93012500
 */
int high_time_update_and_sleep(high_time_t* ht, int32_t curr_time);

#endif//_ZTL_HIGH_TIME_H_
