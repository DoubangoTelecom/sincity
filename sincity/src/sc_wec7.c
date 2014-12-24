#if _WIN32_WCE
// IMPORTANT: OpenSSL was built with "typeof(time_t)==unsigned long" to avoid redefining "time_t" to "uint64_t" (in <stdlib.h> => <crtdefs.h>) we put this #define HERE.
#ifndef _TIME_T_DEFINED
typedef unsigned long  time_t;
#define _TIME_T_DEFINED
#endif

#include "sc_config.h"
#include <Winsock2.h> // timeval
#include <time.h> // struct tm
#include "tsk_time.h" // tsk_gettimeofday

int errno;

struct timeb {
    time_t time;
    unsigned short	millitm;
    short timezone;
    short dstflag;
};

char* getenv(const char* name)
{
    return NULL;
}

void abort()
{
    exit(errno);
}

void _ftime(struct timeb *tp)
{
    struct timeval tv;

    tsk_gettimeofday(&tv, NULL);
    tp->dstflag = 0;
    tp->timezone = 0;
    tp->time = tv.tv_sec;
    tp->millitm = (unsigned short) (tv.tv_usec / 1000);
}

char* strerror(int errnum)
{
    static char __tt[] = "WEC7: strerror() not implemented";
    return __tt;
}

int raise(int sig)
{
    SC_DEBUG_ERROR_EX("WEC7", "Not implemented");
    return -1;
}

long clock(void)
{
    return (long)GetTickCount();
}

#endif /* _WIN32_WCE */
