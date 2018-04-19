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

#if DBG_SYSTIME > 0
#define SYS_DEBUG          OS_DEBUG
#else
#define SYS_DEBUG(...)     do{}while(0)
#endif
/*
********************************************************************************
* 定义模块配置参数
********************************************************************************
*/



/*
********************************************************************************
* 定义静态变量
********************************************************************************
*/
Systime_t g_systime = {0};
volatile Systime_sec_t g_systime_sec = 0;
volatile INT32U g_run_sec = 0;

static void _update_systime_by_rtc(void)
{
	RTC_TimeTypeDef rtcTime = {0};
	RTC_DateTypeDef rtcDate = {0};
	RTC_GetTime(RTC_Format_BIN, &rtcTime);
	RTC_GetDate(RTC_Format_BIN, &rtcDate);
	
	g_systime.tm_year = rtcDate.RTC_Year + 100;
	g_systime.tm_wday = rtcDate.RTC_WeekDay - 1;
	g_systime.tm_mon  = bcd_to_hex_byte(rtcDate.RTC_Month) - 1;
	g_systime.tm_mday = rtcDate.RTC_Date;
	g_systime.tm_hour = rtcTime.RTC_Hours;
	g_systime.tm_min  = rtcTime.RTC_Minutes;
	g_systime.tm_sec  = rtcTime.RTC_Seconds;
}

#if 0
/**
  * @brief  This function handles RTC Alarm interrupt (A and B) request.
  * @param  None
  * @retval None
  */
void RTC_Alarm_IRQHandler(void)
{  
    /* Check on the AlarmA falg and on the number of interrupts per Second (60*8) */
    if(RTC_GetITStatus(RTC_IT_ALRA) != RESET) 
    { 
        /* Clear RTC AlarmA Flags */
        RTC_ClearITPendingBit(RTC_IT_ALRA);
        g_systime_sec++;
        g_run_sec++;
        _update_systime_by_rtc();
    }
    /* Clear the EXTIL line 17 */
    EXTI_ClearITPendingBit(EXTI_Line17);
}
#else
/* 针对1s八次中断，使用这个函数(stm32l100rct6) */
void RTC_Alarm_IRQHandler(void)
{
  static INT8U rtc_alarm_cnt = 1;
  
  /* Check on the AlarmA falg and on the number of interrupts per Second (60*8) */
  if(RTC_GetITStatus(RTC_IT_ALRA) != RESET) 
  { 
    /* Clear RTC AlarmA Flags */
    RTC_ClearITPendingBit(RTC_IT_ALRA);
    
    if (rtc_alarm_cnt++ == 8) {
        /* Increament the counter of Alarma interrupts*/
        rtc_alarm_cnt = 1;

        g_systime_sec++;
        g_run_sec++;
        _update_systime_by_rtc();      
    }
  }
  /* Clear the EXTIL line 17 */
  EXTI_ClearITPendingBit(EXTI_Line17);
}
#endif /* if 0. 2017-7-30 22:24:22 syj */

void os_systime_init(void)
{    
    Systime_t tm = {0};

    if (ds1338_check() == 0) {
        ds1338_readWrite_time(1);
        SYS_DEBUG("ds1338 is valid:%x:%x:%x\r\n", TimeValue.hour, TimeValue.minute, TimeValue.second);

    	tm.tm_year = bcd_to_hex_byte(TimeValue.year) + 100;
    	tm.tm_mon  = bcd_to_hex_byte(TimeValue.month);
    	tm.tm_mday = bcd_to_hex_byte(TimeValue.date);
    	tm.tm_wday = bcd_to_hex_byte(TimeValue.week)-1;
    	tm.tm_hour = bcd_to_hex_byte(TimeValue.hour);
    	tm.tm_min  = bcd_to_hex_byte(TimeValue.minute);
    	tm.tm_sec  = bcd_to_hex_byte(TimeValue.second);

        os_systime_set(&tm);
    } else {
        SYS_DEBUG("ds1338 is invalid\r\n");

        TimeValue.year   = 0x16;
        TimeValue.month  = 0x00;
        TimeValue.date   = 0x17;
        TimeValue.week   = 0x01;
        TimeValue.hour   = 0x01;
        TimeValue.minute = 0x05;
        TimeValue.second = 0x00;
        ds1338_readWrite_time(0);
    }   
}

/*******************************************************************
注释:经过测试，RTC年是从2000开始计算，设置为15就相当于2015年，可以区分
闰年，测试中发现wday是单独就算的，可以随便设置1~7，设置必须在限定的范围
内，比如把日期设置35，过完一天，就会变为36
********************************************************************/

Ret os_systime_set(Systime_t *tm)
{
    Ret ret = RET_OK;
	RTC_TimeTypeDef rtcTime = {0};
	RTC_DateTypeDef rtcDate = {0};

	rtcDate.RTC_Year    = tm->tm_year - 100;  /* RTC取值为0~99,因此设定为2000年开始 */
	rtcDate.RTC_WeekDay = tm->tm_wday + 1;    /* RTC取值为1~7,1为星期一,后面类推 */
    if (tm->tm_mon >= 9) {
    	rtcDate.RTC_Month = tm->tm_mon + 7;   /* RTC取值为BCD码,1为一月,0x10:为十月,0x12为十二月 */
    } else {
    	rtcDate.RTC_Month = tm->tm_mon + 1;   /* RTC取值为BCD码,1为一月,0x10:为十月,0x12为十二月 */
    }
    
	rtcDate.RTC_Date    = tm->tm_mday;        /* RTC取值1-31 */
	rtcTime.RTC_Hours   = tm->tm_hour;        /* RTC取值0~23 */
	rtcTime.RTC_Minutes = tm->tm_min;         /* RTC取值0~59 */
	rtcTime.RTC_Seconds = tm->tm_sec;         /* RTC取值0~59 */
    
	PWR_RTCAccessCmd(ENABLE);
	if (RTC_SetTime(RTC_Format_BIN, &rtcTime) == 0)
		ret = RET_FAIL;
	if (RTC_SetDate(RTC_Format_BIN, &rtcDate) == 0)
		ret = RET_FAIL;

    TimeValue.year   = hex_to_bcd_byte(tm->tm_year - 100);
    TimeValue.month  = hex_to_bcd_byte(tm->tm_mon);
    TimeValue.date   = hex_to_bcd_byte(tm->tm_mday);
    TimeValue.week   = hex_to_bcd_byte(tm->tm_wday+1);
    TimeValue.hour   = hex_to_bcd_byte(tm->tm_hour);
    TimeValue.minute = hex_to_bcd_byte(tm->tm_min);
    TimeValue.second = hex_to_bcd_byte(tm->tm_sec);
    ds1338_readWrite_time(0);
    
	return ret;
}

BOOLEAN os_systime_is_valid(void)
{
    if (g_systime.tm_year < 115) {
        return FALSE;
    } else {
        return TRUE;
    }
}

