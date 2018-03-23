#include <stdlib.h>
#include "ztl_simple_event.h"

#ifdef _MSC_VER
#include <Windows.h>
#endif //_MSC_VER

#ifdef __GNUC__
#include <pthread.h>
#endif //__GNUC__

struct ztl_simevent_st
{
#ifdef __GNUC__
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    bool bsignaled;
#endif // __GNUC__

#ifdef _MSC_VER
    HANDLE cond;
#endif //_MSC_VER
};

#ifdef _MSC_VER
ztl_simevent_t* simevent_create()
{
    ztl_simevent_t* sev;
    sev = (ztl_simevent_t*)malloc(sizeof(ztl_simevent_t));
    sev->cond = CreateEvent(NULL, TRUE, FALSE, NULL);
    return sev;
}

void ztl_simevent_release(ztl_simevent_t* sev)
{
    CloseHandle(sev->cond);
    free(sev);
}

void ztl_simevent_signal(ztl_simevent_t* sev)
{
    PulseEvent(sev->cond);
}

void ztl_simevent_wait(ztl_simevent_t* sev)
{
    WaitForSingleObject(sev->cond, INFINITE);
}
#endif //_MSC_VER

//////////////////////////////////////////////////////////////////////////
#ifdef __GNUC__
ztl_simevent_t* ztl_simevent_create()
{
    simevent_t* sev;
    sev = (simevent_t*)malloc(sizeof(simevent_t));

    sev->bsignaled = false;
    pthread_cond_init(&sev->cond, NULL);
    pthread_mutex_init(&sev->mutex, NULL);
    return sev;
}

void ztl_simevent_release(ztl_simevent_t* sev)
{
    pthread_cond_destroy(&sev->cond);
    pthread_mutex_destroy(&sev->mutex);
}

void ztl_simevent_signal(ztl_simevent_t* sev)
{
    pthread_mutex_lock(&sev->mutex);
    sev->bsignaled = true;
    pthread_cond_signal(&sev->cond);
    pthread_mutex_unlock(&sev->mutex);
}

void ztl_simevent_wait(ztl_simevent_t* sev)
{
    pthread_mutex_lock(&sev->mutex);
    sev->bsignaled = false;
    while (!sev->bsignaled) {
        pthread_cond_wait(&sev->cond, &sev->mutex);
    }
    pthread_mutex_unlock(&sev->mutex);
}
#endif //__GNUC__
