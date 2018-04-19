/********************************************************************************
**
** 文件名:     os_timer.c
** 版权所有:   
** 文件描述:   实现定时器驱动
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2012/08/14 | 苏友江 |  创建该文件
*********************************************************************************/
#include "sys_typedef.h"
#include "os_timer.h"

/*
********************************************************************************
* 定时器结构体
********************************************************************************
*/
typedef struct {
    INT16U          totaltime;                                /* 定时程序的总定时时间，单位100ms */
    INT16U          lefttime;                                 /* 当前的剩余时间, 单位100ms */
    void           *index;                                    /* 定时执行函数特征数据指针 */
    void          (*tmrproc)(void *index);                    /* 定时执行任务的指针 */
} OS_TMR_T;

/*
********************************************************************************
* 定义模块局部变量
********************************************************************************
*/
static OS_TMR_T s_tmrtcb[OS_MAX_TIMER];


/*******************************************************************
** 函数名:     os_timer_get_used_num
** 函数描述:   获取已安装定时器个数
** 参数:       无
** 返回:       已安装定时器个数
********************************************************************/
INT8U os_timer_get_used_num(void)
{
    INT8U id, used;
    
    for (id = 0, used = 0; id < OS_MAX_TIMER; id++) {
        if (s_tmrtcb[id].tmrproc != 0) {
            used++;
        }
    }
    return used;
}

/*******************************************************************
** 函数名:     os_timer_delete
** 函数描述:   删除一个已创建的定时器
** 参数:       [in]  id:        已创建的一个定时器ID
** 返回:       无
********************************************************************/
void os_timer_delete(INT8U id)
{
    if (id >= OS_MAX_TIMER) {
        return;
    }
    
    s_tmrtcb[id].totaltime = 0;
    s_tmrtcb[id].lefttime  = 0;
    s_tmrtcb[id].tmrproc   = 0;
}

/*******************************************************************
** 函数名:     os_timer_left
** 函数描述:   获取定时器剩余的运行时间
** 参数:       [in]  id:        已创建的一个定时器ID
** 返回:       剩余的运行时间, 单位: 100ms
********************************************************************/
INT16U os_timer_left(INT8U id)
{
    if (id >= OS_MAX_TIMER) {
        return 0;
    }
    
    if (s_tmrtcb[id].tmrproc != 0 && s_tmrtcb[id].totaltime > 0) {
        return (s_tmrtcb[id].lefttime);
    } else {
        return 0;
    }
}

/*******************************************************************
** 函数名:     os_timer_is_run
** 函数描述:   判断定时器是否处于运行状态
** 参数:       [in]  id:    已创建的一个定时器ID
** 返回:       true:  定时器处于运行状态
**             false: 定时器处于停止状态
********************************************************************/
BOOLEAN os_timer_is_run(INT8U id)
{
    if (id >= OS_MAX_TIMER) {
        return false;
    }
    
    if (s_tmrtcb[id].tmrproc != 0 && s_tmrtcb[id].totaltime > 0) {
        return true;
    } else {
        return false;
    }
}

/*******************************************************************
** 函数名:     os_timer_create
** 函数描述:   安装一个定时器
** 参数:       [in] index:     定时器标识, 当定时器超时执行超时处理函数时, 该参数作为形参传入处理函数
**             [in] tmrproc:   定时器超时处理函数
** 返回:       定时器ID; 如安装失败, 则返回0xff
** 注意:       安装完定时器还需调用Gps_StartTmr才能启动定时器运行
********************************************************************/
INT8U os_timer_create(void *index, void (*tmrproc)(void *index))
{
    INT8U id;
    
    if (tmrproc == 0) {
        return 0xff;
    }
    
    for (id = 0; id < OS_MAX_TIMER; id++) {                                    /* 查找一个空定时器 */
        if (s_tmrtcb[id].tmrproc == 0) {
            break;
        }
    }
    
    if (id >= OS_MAX_TIMER) {                                                  /* 找不到一个空定时器 */
        return 0xff;
    }

    s_tmrtcb[id].totaltime = 0x00;
    s_tmrtcb[id].lefttime  = 0x00;
    s_tmrtcb[id].index     = index;
    s_tmrtcb[id].tmrproc   = tmrproc;
    return id;
}

/*******************************************************************
** 函数名:     os_timer_start
** 函数描述:   启动定时器
** 参数:       [in]  id:        已创建的一个定时器ID
**             [in]  attrib:    定时器超时时间单位
**             [in]  time:      定时器超时时间
** 返回:       无
********************************************************************/
void os_timer_start(INT8U id, INT16U time)
{
    if ((id >= OS_MAX_TIMER) || (s_tmrtcb[id].tmrproc == 0)) {
        return;
    }

    s_tmrtcb[id].totaltime = time;
    s_tmrtcb[id].lefttime  = s_tmrtcb[id].totaltime;
}

/*******************************************************************
** 函数名:     os_timer_stop
** 函数描述:   停止定时器
** 参数:       [in]  id:        已创建的一个定时器ID
** 返回:       无
********************************************************************/
void os_timer_stop(INT8U id)
{
    if (id >= OS_MAX_TIMER) {
        return;
    }

    s_tmrtcb[id].totaltime = 0;
    s_tmrtcb[id].lefttime  = 0;
}

/*******************************************************************
** 函数名:     os_timer_scan
** 函数描述:   定时器超时后的处理函数
** 参数:       无
** 返回:       无
********************************************************************/
void os_timer_scan(void)
{
    INT8U i;

    for (i = 0; i < OS_MAX_TIMER; i++) {
        if (s_tmrtcb[i].tmrproc != 0 && s_tmrtcb[i].totaltime > 0) {
            if ((--s_tmrtcb[i].lefttime) == 0) {                
                s_tmrtcb[i].lefttime = s_tmrtcb[i].totaltime;
                s_tmrtcb[i].tmrproc(s_tmrtcb[i].index);
            }
        }
    }
}

/*******************************************************************
** 函数名:     os_timer_init
** 函数描述:   定时器模块初始化函数
** 参数:       无
** 返回:       无
********************************************************************/
void os_timer_init(void)
{
    INT8U i;
    
    for (i = 0; i < OS_MAX_TIMER; i++) {
        s_tmrtcb[i].totaltime = 0;
        s_tmrtcb[i].tmrproc   = 0;
    }
}

