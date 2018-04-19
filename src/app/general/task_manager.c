/********************************************************************************
**
** 文件名:     task_manager.c
** 版权所有:   (c) 2013-2015 
** 文件描述:   任务管理
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**|    日期    |  作者  |  修改记录
**===============================================================================
**| 2015/09/14 | 苏友江 |  创建该文件
********************************************************************************/
#define GLOBALS_TASK_PARA     1
#include "bsp.h"
#include "sys_includes.h"

#if DBG_GENERAL > 0
#define SYS_DEBUG          OS_DEBUG
#else
#define SYS_DEBUG(...)     do{}while(0)
#endif
/*
********************************************************************************
* 结构定义
********************************************************************************
*/


typedef struct {
    INT16U sunrise;
    INT16U sunset;
    Task_t *task;
} Priv_t;


/*
********************************************************************************
* 静态变量
********************************************************************************
*/
static INT8U  s_tasktmr;
static Priv_t s_priv;



/*******************************************************************
** 函数名:     _task_manager_tmr
** 函数描述:   task manager定时器
** 参数:       [in] index  : 定时器参数
** 返回:       无
********************************************************************/
static void _task_manager_tmr(void *index)
{   
	INT8U i, j, m = 0, n = 0;
	BOOLEAN do_flog = FALSE;
    INT32U now, temp, near = 0;                                                /* 一天中0点到此刻的分钟数 */
    Lamp_event_t event;
    static INT8U s_lamp = LAMP1_INDEX;
    static INT32U lasttime[LAMP_NUM_MAX] = {0xffffffff};

    if (!os_systime_is_valid()) {
        return;
    }

    if (++s_lamp >= LAMP_NUM_MAX) {
        s_lamp = LAMP1_INDEX;
    }
    now = g_systime.tm_hour*60 + g_systime.tm_min;

    if (s_priv.task->mode == TASK_TIME) {
    	for (i = 0; i < s_priv.task->time_num[s_lamp]; i++) {
    		for (j = 0; j < s_priv.task->time[s_lamp][i].num; j++) {
                if (s_priv.task->time[s_lamp][i].type.ch.dt != 0) {
                    continue;
                }
                
				if ((s_priv.task->time[s_lamp][i].type.type && (0x01<<g_systime.tm_wday)) == 0) {
                    continue;
				}
                    
				temp = s_priv.task->time[s_lamp][i].action[j].time.hour*60 + s_priv.task->time[s_lamp][i].action[j].time.minute;
                
				if (temp <= now) {
					if (temp >= near) {
						near = temp;
                        m = i;
                        n = j;
						do_flog = TRUE;
					}
				}
    		}
    	}

        if (do_flog && (lasttime[s_lamp] != near)) {
            lasttime[s_lamp] = near;
            event.event   = LAMP_EVENT_CENTER;
            event.ctl     = s_priv.task->time[s_lamp][m].action[n].ctl;
            event.dimming = s_priv.task->time[s_lamp][m].action[n].dimming;
        } else {
            do_flog = FALSE;                                                   /* 这个时间点已经执行过了，就不在执行 */
        }
    } else if (s_priv.task->mode == TASK_LONG_LAT) {
        static INT8U day = 0xff, ctl = 0xff;
        
        if (g_systime.tm_mday != day) {
            struct tm curtime;
            Systime_sec_t temp_sec;
                
            day = g_systime.tm_mday;
            /* 利用经纬度，在算法中计算出当天的日出和日落时间 */
            SYS_DEBUG("<---Year:%d, Mon:%d, Day:%d>\n", g_systime.tm_year, g_systime.tm_mon, g_systime.tm_mday);
            curtime.tm_mday = g_systime.tm_mday;
            curtime.tm_mon  = g_systime.tm_mon;
            curtime.tm_year = g_systime.tm_year;
            
    		SYS_DEBUG("<---lon:%f, lat:%f>\n", s_priv.task->long_lat.lon, s_priv.task->long_lat.lat);
            calcSunTime(&curtime, s_priv.task->long_lat.lon, s_priv.task->long_lat.lat);

            temp_sec = getSunrise();
            s_priv.sunrise  = (temp_sec%(24*60*60))/60;                        /* 转换为分钟 */
            temp_sec = getSunset();
            s_priv.sunset   = (temp_sec%(24*60*60))/60;
        }
        
		SYS_DEBUG("<---Now:[%d:%d], Sunrise:[%d:%d], Sunset:[%d:%d]>\n", now/60, now%60, s_priv.sunrise/60, s_priv.sunrise%60, s_priv.sunset/60, s_priv.sunset%60);

        if ((now >= s_priv.sunrise) && (now < s_priv.sunset)) {                /* 白天光灯 */
            if (ctl != LAMP_CLOSE) {
                do_flog       = TRUE;
                ctl           = LAMP_CLOSE;
                event.event   = LAMP_EVENT_CENTER;
                event.ctl     = ctl;
                event.dimming = 0;
            }
        } else {                                                               /* 夜晚开灯 */
            if (ctl != LAMP_OPEN) {
                do_flog       = TRUE;
                ctl           = LAMP_OPEN;
                event.event   = LAMP_EVENT_CENTER;
                event.ctl     = ctl;
                event.dimming = 100;
            }
        }
    }

    if (do_flog) {
        lamp_event_create((Lamp_e)s_lamp, &event);
    }
}

/*******************************************************************
** 函数名:      _para_change_informer
** 函数描述:    参数变化
** 参数:        [in] reason
** 返回:        true or false
********************************************************************/
static void _para_change_informer(INT8U reason)
{
    if (reason != PP_REASON_STORE) {
        return;
    }

    public_para_manager_read_by_id(TASK_, (INT8U*)s_priv.task, sizeof(Task_t));
}

/*******************************************************************
** 函数名:      task_manager_init
** 函数描述:    task初始化
** 参数:        无
** 返回:        无
********************************************************************/
void task_manager_init(void)
{
    memset(&s_priv, 0, sizeof(Priv_t));
    s_priv.task = &g_task_para;
    public_para_manager_read_by_id(TASK_, (INT8U*)s_priv.task, sizeof(Task_t));

    public_para_manager_reg_change_informer(TASK_, _para_change_informer);
        
    s_tasktmr = os_timer_create(0, _task_manager_tmr);
    os_timer_start(s_tasktmr, 1*SECOND);
}

