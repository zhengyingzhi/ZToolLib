#ifndef _ZTL_ERRORS_H_
#define _ZTL_ERRORS_H_

// #include <stdint.h>

#define ZTL_ERR_BASE                    (-1000)
#define ZTL_OK                          (0)
#define ZTL_SUCCESS                     (0)
#define ZTL_NoError                     (0)
#define ZTL_ERR_AlreadyExist            (1)
#define ZTL_ERR_AlreadyInited           (2)
#define ZTL_ERR_AlreadyRunning          (3)


#define ZTL_ERR_NullType                (ZTL_ERR_BASE - 1)
#define ZTL_ERR_NotImpl                 (ZTL_ERR_BASE - 2)
#define ZTL_ERR_NotSet                  (ZTL_ERR_BASE - 3)
#define ZTL_ERR_InvalParam              (ZTL_ERR_BASE - 4)
#define ZTL_ERR_AllocFailed             (ZTL_ERR_BASE - 5)
#define ZTL_ERR_NotCreated              (ZTL_ERR_BASE - 6)
#define ZTL_ERR_NotInited               (ZTL_ERR_BASE - 7)
#define ZTL_ERR_NotStarted              (ZTL_ERR_BASE - 8)
#define ZTL_ERR_NotStartThread          (ZTL_ERR_BASE - 9)
#define ZTL_ERR_NotRunning              (ZTL_ERR_BASE - 10)
#define ZTL_ERR_NotRegistered           (ZTL_ERR_BASE - 11)
#define ZTL_ERR_NotFound                (ZTL_ERR_BASE - 12)
#define ZTL_ERR_QueueFull               (ZTL_ERR_BASE - 13)
#define ZTL_ERR_Empty                   (ZTL_ERR_BASE - 14)
#define ZTL_ERR_OutOfEVID               (ZTL_ERR_BASE - 15)
#define ZTL_ERR_OutOfRange              (ZTL_ERR_BASE - 16)
#define ZTL_ERR_OutOfMem                (ZTL_ERR_BASE - 17)

#define ZTL_ERR_BadFD                   (ZTL_ERR_BASE - 20)
#define ZTL_ERR_NotConnected            (ZTL_ERR_BASE - 21)
#define ZTL_ERR_Timeout                 (ZTL_ERR_BASE - 22)


#endif//_ZTL_ERRORS_H_
