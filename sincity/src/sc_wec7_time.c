#if _WIN32_WCE
// https://qt.gitorious.org/qt/qt/raw/325b6ef5d68a7066df9fb6cf48474257e3d57ea9:src/3rdparty/ce-compat/ce_time.c

//
// Date to string conversion
//
// Copyright (C) 2002 Michael Ringgaard. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. Neither the name of the project nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.
//

/////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                         //
//                                         time()                                          //
//                                                                                         //
/////////////////////////////////////////////////////////////////////////////////////////////

// IMPORTANT: OpenSSL was built with "typeof(time_t)==unsigned long" to avoid redefining "time_t" to "uint64_t" (in <stdlib.h> => <crtdefs.h>) we put this #define HERE.
#ifndef _TIME_T_DEFINED
typedef unsigned long  time_t;
#define _TIME_T_DEFINED
#endif

#include <windows.h>
#include <time.h>

time_t mktime(struct tm *t);

time_t time(time_t* timer)
{
    SYSTEMTIME systime;
    struct tm tmtime;
    time_t tt;

    GetLocalTime(&systime);

    tmtime.tm_year = systime.wYear-1900;
    tmtime.tm_mon = systime.wMonth-1;
    tmtime.tm_mday = systime.wDay;
    tmtime.tm_wday = systime.wDayOfWeek;
    tmtime.tm_hour = systime.wHour;
    tmtime.tm_min = systime.wMinute;
    tmtime.tm_sec = systime.wSecond;

    tt = mktime(&tmtime);

    if(timer) {
        *timer = tt;
    }

    return tt;
}


/////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                         //
//                                        mktime()                                         //
//                                                                                         //
/////////////////////////////////////////////////////////////////////////////////////////////

static int month_to_day[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

time_t mktime(struct tm *t)
{
    short  month, year;
    time_t result;

    month = t->tm_mon;
    year = t->tm_year + month / 12 + 1900;
    month %= 12;
    if (month < 0) {
        year -= 1;
        month += 12;
    }
    result = (year - 1970) * 365 + (year - 1969) / 4 + month_to_day[month];
    result = (year - 1970) * 365 + month_to_day[month];
    if (month <= 1) {
        year -= 1;
    }
    result += (year - 1968) / 4;
    result -= (year - 1900) / 100;
    result += (year - 1600) / 400;
    result += t->tm_mday;
    result -= 1;
    result *= 24;
    result += t->tm_hour;
    result *= 60;
    result += t->tm_min;
    result *= 60;
    result += t->tm_sec;
    return(result);
}


/////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                         //
//                                      gmtime()                                           //
//                                                                                         //
/////////////////////////////////////////////////////////////////////////////////////////////


static struct tm mytm;

static int DMonth[13] = { 0,31,59,90,120,151,181,212,243,273,304,334,365 };
static int monthCodes[12] = { 6, 2, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };


static int calcDayOfWeek(const struct tm* nTM)
{
    int day;

    day = (nTM->tm_year%100);
    day += day/4;
    day += monthCodes[nTM->tm_mon];
    day += nTM->tm_mday;
    while(day>=7) {
        day -= 7;
    }

    return day;
}


struct tm * gmtime(const time_t *timer)
{
    unsigned long x = *timer;
    int imin, ihrs, iday, iyrs;
    int sec, min, hrs, day, mon, yrs;
    int lday, qday, jday, mday;


    imin = x / 60;							// whole minutes since 1/1/70
    sec = x - (60 * imin);					// leftover seconds
    ihrs = imin / 60;						// whole hours since 1/1/70
    min = imin - 60 * ihrs;					// leftover minutes
    iday = ihrs / 24;						// whole days since 1/1/70
    hrs = ihrs - 24 * iday;					// leftover hours
    iday = iday + 365 + 366; 				// whole days since 1/1/68
    lday = iday / (( 4* 365) + 1);			// quadyr = 4 yr period = 1461 days
    qday = iday % (( 4 * 365) + 1);			// days since current quadyr began
    if(qday >= (31 + 29)) {				// if past feb 29 then
        lday = lday + 1;    // add this quadyr’s leap day to the
    }
    // # of quadyrs (leap days) since 68
    iyrs = (iday - lday) / 365;				// whole years since 1968
    jday = iday - (iyrs * 365) - lday;		// days since 1 /1 of current year.
    if(qday <= 365 && qday >= 60) {		// if past 2/29 and a leap year then
        jday = jday + 1;    // add a leap day to the # of whole
    }
    // days since 1/1 of current year
    yrs = iyrs + 1968;						// compute year
    mon = 13;								// estimate month ( +1)
    mday = 366;								// max days since 1/1 is 365
    while(jday < mday) {					// mday = # of days passed from 1/1
        // until first day of current month
        mon = mon - 1;						// mon = month (estimated)
        mday = DMonth[mon];					// # elapsed days at first of ”mon”
        if((mon > 2) && (yrs % 4) == 0) {	// if past 2/29 and leap year then
            mday = mday + 1;    // add leap day
        }
        // compute month by decrementing
    }										// month until found

    day = jday - mday + 1;					// compute day of month

    mytm.tm_sec = sec;
    mytm.tm_min = min;
    mytm.tm_hour = hrs;
    mytm.tm_mday = day;
    mytm.tm_mon = mon;
    mytm.tm_year = yrs - 1900;

    mytm.tm_wday = calcDayOfWeek(&mytm);
    mytm.tm_yday = jday;
    mytm.tm_isdst = 0;

    return &mytm;
}


/////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                         //
//                            localtime() - simply using gmtime()                          //
//                                                                                         //
/////////////////////////////////////////////////////////////////////////////////////////////


struct tm * localtime(const time_t *timer)
{
    return gmtime(timer);
}

#endif /* _WIN32_WCE */
