/********************************************************************************
**
** 文件名:     os_systime.c
** 版权所有:   (c) 2013-2015 
** 文件描述:   实时时间管理
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**|    日期    |  作者  |  修改记录
**===============================================================================
**| 2015/09/06 | 苏友江 |  创建该文件
********************************************************************************/
#include "bsp.h"
#include "sys_includes.h"


/*
********************************************************************************
* 定义模块配置参数
********************************************************************************
*/

#define YEAR0                   1900
#define EPOCH_YR                1970
#define YEARSIZE(year)          (LEAPYEAR(year) ? 366 : 365)
#define LEAPYEAR(year)          (!((year) % 4) && (((year) % 100) || !((year) % 400)))
#define SECS_DAY                (24L * 60L * 60L)
#define TIME_MAX                2147483647L

/*
********************************************************************************
* 定义静态变量
********************************************************************************
*/
const int _ytab[2][12] = 
{
  {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
  {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};

long _timezone = 0;         /* Difference in seconds between GMT and local time */
long _dstbias = 0;          /* Offset for Daylight Saving Time */

Systime_t g_systime = {0};
volatile Systime_sec_t g_systime_sec = 0;
INT32U g_run_sec = 0;

static INT8U s_updatetmr;

static Systime_sec_t _mktime_xrt(Systime_t *tmbuf)
{
	long day, year;
	int tm_year;
	int yday, month;
	long seconds;
	int overflow;
	long dst;

	tmbuf->tm_min += tmbuf->tm_sec / 60;
	tmbuf->tm_sec %= 60;
	if (tmbuf->tm_sec < 0) 
	{
		tmbuf->tm_sec += 60;
		tmbuf->tm_min--;
	}
	tmbuf->tm_hour += tmbuf->tm_min / 60;
	tmbuf->tm_min = tmbuf->tm_min % 60;
	if (tmbuf->tm_min < 0) 
	{
		tmbuf->tm_min += 60;
		tmbuf->tm_hour--;
	}
	day = tmbuf->tm_hour / 24;
	tmbuf->tm_hour= tmbuf->tm_hour % 24;
	if (tmbuf->tm_hour < 0) 
	{
		tmbuf->tm_hour += 24;
		day--;
	}
	tmbuf->tm_year += tmbuf->tm_mon / 12;
	tmbuf->tm_mon %= 12;
	if (tmbuf->tm_mon < 0) 
	{
		tmbuf->tm_mon += 12;
		tmbuf->tm_year--;
	}
	day += (tmbuf->tm_mday - 1);
	while (day < 0) 
	{
		if(--tmbuf->tm_mon < 0) 
		{
			tmbuf->tm_year--;
	 		tmbuf->tm_mon = 11;
		}
		day += _ytab[LEAPYEAR(YEAR0 + tmbuf->tm_year)][tmbuf->tm_mon];
	}
	while (day >= _ytab[LEAPYEAR(YEAR0 + tmbuf->tm_year)][tmbuf->tm_mon]) 
	{
		day -= _ytab[LEAPYEAR(YEAR0 + tmbuf->tm_year)][tmbuf->tm_mon];
		if (++(tmbuf->tm_mon) == 12) 
		{
			tmbuf->tm_mon = 0;
			tmbuf->tm_year++;
		}
	}
	tmbuf->tm_mday = day + 1;
	year = EPOCH_YR;
	if (tmbuf->tm_year < year - YEAR0) return (Systime_sec_t) -1;
	seconds = 0;
	day = 0;						// Means days since day 0 now
	overflow = 0;

	// Assume that when day becomes negative, there will certainly
	// be overflow on seconds.
	// The check for overflow needs not to be done for leapyears
	// divisible by 400.
	// The code only works when year (1970) is not a leapyear.
	tm_year = tmbuf->tm_year + YEAR0;

	if (TIME_MAX / 365 < tm_year - year) overflow++;
	day = (tm_year - year) * 365;
	if (TIME_MAX - day < (tm_year - year) / 4 + 1) overflow++;
	day += (tm_year - year) / 4 + ((tm_year % 4) && tm_year % 4 < year % 4);
	day -= (tm_year - year) / 100 + ((tm_year % 100) && tm_year % 100 < year % 100);
	day += (tm_year - year) / 400 + ((tm_year % 400) && tm_year % 400 < year % 400);

	yday = month = 0;
	while (month < tmbuf->tm_mon)
	{
		yday += _ytab[LEAPYEAR(tm_year)][month];
		month++;
	}
	yday += (tmbuf->tm_mday - 1);
	if (day + yday < 0) overflow++;
		day += yday;

	tmbuf->tm_yday = yday;
	tmbuf->tm_wday = (day + 4) % 7;				// Day 0 was thursday (4)

	seconds = ((tmbuf->tm_hour * 60L) + tmbuf->tm_min) * 60L + tmbuf->tm_sec;

	if ((TIME_MAX - seconds) / SECS_DAY < day) overflow++;
		seconds += day * SECS_DAY;

	// Now adjust according to timezone and daylight saving time
	if (((_timezone > 0) && (TIME_MAX - _timezone < seconds))
	  || ((_timezone < 0) && (seconds < -_timezone)))
		  overflow++;
	seconds += _timezone;

	if (tmbuf->tm_isdst)
		dst = _dstbias;
	else 
		dst = 0;

	if (dst > seconds) overflow++;		// dst is always non-negative
		seconds -= dst;

	if (overflow) return (Systime_sec_t) -1;

	if ((Systime_sec_t) seconds != seconds) return (Systime_sec_t) -1;
		return (Systime_sec_t) seconds;
}

static void _update_systime_by_rtc(void)
{
	RTC_TimeTypeDef rtcTime = {0};
	RTC_DateTypeDef rtcDate = {0};
	RTC_GetTime(RTC_Format_BIN, &rtcTime);
	RTC_GetDate(RTC_Format_BIN, &rtcDate);
	
	g_systime.tm_year = rtcDate.RTC_Year+100;
	g_systime.tm_wday = rtcDate.RTC_WeekDay - 1;
	g_systime.tm_mon  = rtcDate.RTC_Month - 1;
	g_systime.tm_mday = rtcDate.RTC_Date;
	g_systime.tm_hour = rtcTime.RTC_Hours;
	g_systime.tm_min  = rtcTime.RTC_Minutes;
	g_systime.tm_sec  = rtcTime.RTC_Seconds;
	g_systime_sec = _mktime_xrt(&g_systime);
}

/*******************************************************************
** 函数名:     _update_tmr_proc
** 函数描述:   更新时间定时器函数
** 参数:       [in]  pdata:  保留未用
** 返回:       无
********************************************************************/
static void _update_tmr_proc(void *pdata)
{      
    static Systime_sec_t temptime = 0;
    pdata = pdata;

    if (g_systime_sec == temptime) {
        return;
    }
    temptime = g_systime_sec;
    os_systime_convert((const Systime_sec_t*)&g_systime_sec, &g_systime);

    //printf("time:%d %d %d  %d:%d:%d\n", g_systime.tm_year, g_systime.tm_mon, g_systime.tm_mday, g_systime.tm_hour, g_systime.tm_min, g_systime.tm_sec);
    if (g_systime_sec%60 == 0) {                                               /* 一分钟更新一次时间 */
        _update_systime_by_rtc();
    }
}

void os_systime_init(void)
{
    _update_systime_by_rtc();
        
    s_updatetmr = os_timer_create( 0, _update_tmr_proc);
    os_timer_start(s_updatetmr, 5*MILTICK);
}

Ret os_systime_set(Systime_t *tm)
{
	RTC_TimeTypeDef rtcTime = {0};
	RTC_DateTypeDef rtcDate = {0};

	rtcDate.RTC_Year    = tm->tm_year;
	rtcDate.RTC_WeekDay = tm->tm_wday;
	rtcDate.RTC_Month   = tm->tm_mon;
	rtcDate.RTC_Date    = tm->tm_mday;
	rtcTime.RTC_Hours   = tm->tm_hour;
	rtcTime.RTC_Minutes = tm->tm_min;
	rtcTime.RTC_Seconds = tm->tm_sec;
    
	PWR_RTCAccessCmd(ENABLE);
	if (RTC_SetTime(RTC_Format_BIN, &rtcTime) == 0)
		return RET_FAIL;
	if (RTC_SetDate(RTC_Format_BIN, &rtcDate) == 0)
		return RET_FAIL;
    
    _update_systime_by_rtc();
	return RET_OK;
}

Systime_t *os_systime_convert(const Systime_sec_t *timer, Systime_t *tmbuf)
{
    Systime_sec_t time = *timer;
    unsigned long dayclock, dayno;
    int year = EPOCH_YR;

    dayclock = (unsigned long) time % SECS_DAY;
    dayno = (unsigned long) time / SECS_DAY;

    tmbuf->tm_sec = dayclock % 60;
    tmbuf->tm_min = (dayclock % 3600) / 60;
    tmbuf->tm_hour = dayclock / 3600;
    tmbuf->tm_wday = (dayno + 4) % 7; // Day 0 was a thursday
    while (dayno >= (unsigned long) YEARSIZE(year)) 
    {
        dayno -= YEARSIZE(year);
        year++;
    }
    tmbuf->tm_year = year - YEAR0;
    tmbuf->tm_yday = dayno;
    tmbuf->tm_mon = 0;
    while (dayno >= (unsigned long) _ytab[LEAPYEAR(year)][tmbuf->tm_mon]) 
    {
        dayno -= _ytab[LEAPYEAR(year)][tmbuf->tm_mon];
        tmbuf->tm_mon++;
    }
    tmbuf->tm_mday = dayno + 1;
    tmbuf->tm_isdst = 0;
    //tmbuf->tm_gmtoff = 0;
    //tmbuf->tm_zone = "UTC";
    return tmbuf;
}

