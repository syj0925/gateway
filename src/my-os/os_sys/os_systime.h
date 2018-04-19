/********************************************************************************
**
** 文件名:     os_systime.h
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
#ifndef SYSTIME_H
#define SYSTIME_H



typedef INT32U Systime_sec_t;

typedef struct {
	INT8S tm_sec;	/* 秒 - 取值区间为[0,59] */
	INT8S tm_min;	/* 分 - 取值区间为[0,59] */
	INT8S tm_hour;	/* 时 - 取值区间为[0,23] */
	INT8S tm_mday;	/* 一个月中的日期 - 取值区间为[1,31] */
	INT8S tm_mon;	/* 月份（从一月开始，0代表一月） - 取值区间为[0,11] */
	INT8S tm_year;	/* 年份，其值从1900开始 */
	INT8S tm_wday;	/* 星期–取值区间为[0,6]，其中0代表星期天，1代表星期一，以此类推 */
	INT16S tm_yday;	/* 从每年的1月1日开始的天数–取值区间为[0,365]，其中0代表1月1日，1代表1月2日，以此类推 */
	INT8S tm_isdst;	/* 夏令时标识符，实行夏令时的时候，tm_isdst为正。不实行夏令时的进候，tm_isdst为0；不了解情况时，tm_isdst()为负。*/ 
} Systime_t;

#if 0/* 标准库中的定义 */
    #include <time.h>
    
    int tm_sec;   /* seconds after the minute, 0 to 60
                     (0 - 60 allows for the occasional leap second) */
    int tm_min;   /* minutes after the hour, 0 to 59 */
    int tm_hour;  /* hours since midnight, 0 to 23 */
    int tm_mday;  /* day of the month, 1 to 31 */
    int tm_mon;   /* months since January, 0 to 11 */
    int tm_year;  /* years since 1900 */
    int tm_wday;  /* days since Sunday, 0 to 6 */
    int tm_yday;  /* days since January 1, 0 to 365 */
    int tm_isdst; /* Daylight Savings Time flag */
#endif /* if 0. 2015-10-3 14:33:10 suyoujiang */

extern Systime_t g_systime;
extern volatile Systime_sec_t g_systime_sec;
extern volatile INT32U g_run_sec;


void os_systime_init(void);

Ret os_systime_set(Systime_t *tm);

BOOLEAN os_systime_is_valid(void);
//Systime_t *os_systime_convert(const Systime_sec_t *timer, Systime_t *tmbuf);

#endif

