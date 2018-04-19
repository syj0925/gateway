/********************************************************************************
**
** 文件名:     alarm_manager.c
** 版权所有:   (c) 2013-2015 
** 文件描述:   报警管理
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**|    日期    |  作者  |  修改记录
**===============================================================================
**| 2015/09/26 | 苏友江 |  创建该文件
********************************************************************************/
#include "bsp.h"
#include "sys_includes.h"
#include "alarm_v_leakage.h"
#include "alarm_turn_on_fault.h"
#include "alarm_turn_off_fault.h"

#if DBG_ALARM > 0
#define SYS_DEBUG          OS_DEBUG
#else
#define SYS_DEBUG(...)     do{}while(0)
#endif
/*
********************************************************************************
*宏定义
********************************************************************************
*/
#define RESEND_NUM      3
#define WAIT_TIME       30

/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
typedef struct {
    BOOLEAN report;
    INT16U  flowseq;
    INT32U  lasttime;
    INT32U  status;
    Alarm_t *plist[ALARM_ID_MAX];
} Priv_t;

/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/
static INT8U s_alarmtmr;
static Priv_t s_priv;


static void _alarm_report_ack(INT16U flowseq, Comm_pkt_send_t *packet, INT8U result)
{
    if (flowseq != s_priv.flowseq) {
        return;
    }

    SYS_DEBUG("<_alarm_report_ack:%d>\n", result);
    if (result == _SUCCESS) {
        s_priv.report = FALSE;
    }
}

static void _alarm_report(void)
{
    INT8U *psendbuf;
    Stream_t wstrm;
    Comm_pkt_send_t send_pkt;

    psendbuf = comm_protocol_asm_status(&wstrm);
    if (psendbuf != NULL) {
        send_pkt.len   = stream_get_len(&wstrm);
        send_pkt.pdata = psendbuf;
        send_pkt.msgid = 0x0203;
        s_priv.flowseq = comm_send_listsend(&send_pkt, RESEND_NUM, WAIT_TIME, _alarm_report_ack);
        mem_free(psendbuf);
    }
}

/*******************************************************************
** 函数名:     _alarm_manager_tmr
** 函数描述:   alarm manager定时器
** 参数:       [in] index  : 定时器参数
** 返回:       无
********************************************************************/
static void _alarm_manager_tmr(void *index)
{    
    INT8U i;
    INT32U temp = 0;
    index = index;
    
    os_timer_start(s_alarmtmr, 1*SECOND);
    for (i = 0; i < ALARM_ID_MAX; i++) {
        if (s_priv.plist[i] == NULL) {
            continue;
        }

        if (alarm_scan(s_priv.plist[i]) == 1) {
            temp |= 0x00000001<<i;
        } else {
            //temp &= ~(0x00000001<<i);
        }
    }

    if (temp != s_priv.status) {
        s_priv.status = temp;
        s_priv.report = TRUE;
        _alarm_report();
        s_priv.lasttime = g_run_sec + 300;
    }

    if (s_priv.report == TRUE) {
        if (g_run_sec > s_priv.lasttime) {
            _alarm_report();
            s_priv.lasttime = g_run_sec + 300;
        }
    }
}

/*******************************************************************
** 函数名:      alarm_manager_init
** 函数描述:    alarm初始化
** 参数:        无
** 返回:        无
********************************************************************/
void alarm_manager_init(void)
{
    memset(&s_priv, 0, sizeof(Priv_t));

    #ifdef ALARM_DEF
    #undef ALARM_DEF
    #endif

    #define ALARM_DEF(_TYPE_ID_, _CREATE_, _PERIOD_, _MONITOR_) \
       s_priv.plist[_TYPE_ID_] = _CREATE_(_TYPE_ID_);

    #include "alarm_reg.def"

    s_alarmtmr = os_timer_create(0, _alarm_manager_tmr);
    os_timer_start(s_alarmtmr, 30*SECOND);                                     /* 延时30s，在去检测报警，避免刚上电后，一些电能数据有些异常，而导致误报警 */
}

/*******************************************************************
** 函数名:      alarm_get_status
** 函数描述:    获取报警状态
** 参数:        none
** 返回:        status
********************************************************************/
INT32U alarm_get_status(void)
{
    return s_priv.status;
}

