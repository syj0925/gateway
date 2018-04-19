/********************************************************************************
**
** 文件名:     lamp_manager.c
** 版权所有:   (c) 2013-2015
** 文件描述:   灯管理模块
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**|    日期    |  作者  |  修改记录
**===============================================================================
**| 2015/08/29 | 苏友江 |  创建该文件
********************************************************************************/
#include "bsp.h"
#include "sys_includes.h"

#if DBG_GENERAL > 0
#define SYS_DEBUG          OS_DEBUG
#else
#define SYS_DEBUG(...)     do{}while(0)
#endif
/*
********************************************************************************
* 参数配置
********************************************************************************
*/
#define EN_STEP_DIMMING     1
#define DIMMING_STEP        5


/*
********************************************************************************
* 结构定义
********************************************************************************
*/
typedef struct {
    BOOLEAN executed;       /* 执行标志位，0表示未执行，1表示已执行 */
    INT8U prio_event;       /* 当前优先级最高的事件，0xff表示当前没有事件 */
    INT8U ctl;
    INT8U dimming;
} Lamp_st_t;


typedef struct {
    Lamp_st_t lamp_st[LAMP_NUM_MAX];
} Priv_t;

/*
********************************************************************************
* 静态变量
********************************************************************************
*/
static INT8U s_lamptmr;
static Lamp_event_t s_event[LAMP_NUM_MAX][LAMP_EVENT_MAX];
static Priv_t s_priv;

static void _lamp_ctl(INT8U lamp, INT8U ctl, INT8U dimming)
{
    switch (lamp) {
        case LAMP1_INDEX:
            if (ctl == LAMP_OPEN) {                                            /* 开灯和调光 */
                GPIO_ResetBits(GPIOC, GPIO_Pin_4);
                pwm_dimming(lamp, dimming);
                dac_dimming(lamp, dimming);
            } else if (ctl == LAMP_CLOSE) {
                GPIO_SetBits(GPIOC, GPIO_Pin_4);                               /* 关灯和调光0 */
                pwm_dimming(lamp, 0);
                dac_dimming(lamp, 0);
            }
            break;
        default:
            break;
    }
}

/*******************************************************************
** 函数名:      _update_lamp_status
** 函数描述:    按照灯事件表更新当前状态
** 参数:        none
** 返回:        none
********************************************************************/
static void _update_lamp_status(void)
{
    INT8U i, j;

    for (i = 0; i < LAMP_NUM_MAX; i++) {
        s_priv.lamp_st[i].executed = 0;
        s_priv.lamp_st[i].prio_event = 0xff;
        
        for (j = 0; j < LAMP_EVENT_MAX; j++) {
            if (s_event[i][j].event == 0xff) {
                continue;
            }
            
            /* 记录当前优先级最高的事件 */
            s_priv.lamp_st[i].prio_event = j;
            if (s_event[i][j].ctl == LAMP_OPEN) {
                s_priv.lamp_st[i].ctl = LAMP_OPEN;
            }
            break;
        }
        
    }
}

/*******************************************************************
** 函数名:     _lamp_manager_tmr
** 函数描述:   lamp manager定时器
** 参数:       [in] index  : 定时器参数
** 返回:       无
********************************************************************/
static void _lamp_manager_tmr(void *index)
{    
    INT8U i;
    index = index;

    for (i = 0; i < LAMP_NUM_MAX; i++) {
        if (s_priv.lamp_st[i].executed) {
            continue;
        }
        if (s_priv.lamp_st[i].prio_event == 0xff) {
            s_priv.lamp_st[i].executed = 1;
            s_priv.lamp_st[i].ctl = LAMP_CLOSE;
            s_priv.lamp_st[i].dimming = 0;
        } else {
            #if EN_STEP_DIMMING
            if (s_priv.lamp_st[i].dimming + DIMMING_STEP < s_event[i][s_priv.lamp_st[i].prio_event].dimming) {
                s_priv.lamp_st[i].dimming += DIMMING_STEP;
            } else if (s_priv.lamp_st[i].dimming > s_event[i][s_priv.lamp_st[i].prio_event].dimming + DIMMING_STEP) {
                s_priv.lamp_st[i].dimming -= DIMMING_STEP;
            } else {
                s_priv.lamp_st[i].executed = 1;
                s_priv.lamp_st[i].ctl     = s_event[i][s_priv.lamp_st[i].prio_event].ctl;
                s_priv.lamp_st[i].dimming = s_event[i][s_priv.lamp_st[i].prio_event].dimming;
            }
            #else
            s_priv.lamp_st[i].executed = 1;
            s_priv.lamp_st[i].ctl     = s_event[i][s_priv.lamp_st[i].prio_event].ctl;
            s_priv.lamp_st[i].dimming = s_event[i][s_priv.lamp_st[i].prio_event].dimming;
            #endif
        }
        
        _lamp_ctl(i, s_priv.lamp_st[i].ctl, s_priv.lamp_st[i].dimming);
        //SYS_DEBUG("lamp no:[%d], ctl:[%d], dim:[%d]\n", i, s_priv.lamp_st[i].ctl, s_priv.lamp_st[i].dimming);
    }

    for (i = 0; i < LAMP_NUM_MAX; i++) {
        if (!s_priv.lamp_st[i].executed) {
            os_timer_start(s_lamptmr, MILTICK);
            return;
        }
    }
    os_timer_stop(s_lamptmr);
}


static void _handle_0x8201(Comm_pkt_recv_t *packet)
{
    Stream_t rstrm;
    INT8U i, lamp_num, lamp_index;
    Lamp_event_t event;

    stream_init(&rstrm, packet->pdata, packet->len);
    lamp_num = stream_read_byte(&rstrm);
    //event.event = LAMP_EVENT_CENTER;
    event.event = LAMP_EVENT_ALARM;
    for (i = 0; i < lamp_num; i++) {
        lamp_index = stream_read_byte(&rstrm);
        event.ctl  = stream_read_byte(&rstrm);
        if (event.ctl == LAMP_OPEN) {
            event.dimming = stream_read_byte(&rstrm);
        } else {
            event.dimming = 0;
        }

        lamp_event_create((Lamp_e)lamp_index, &event);
    }

    if (packet->ack) {
        INT8U *psendbuf;
        Stream_t wstrm;
        Comm_pkt_send_t send_pkt;

        psendbuf = comm_protocol_asm_status(&wstrm);
        if (psendbuf != NULL) {
            send_pkt.len   = stream_get_len(&wstrm);
            send_pkt.pdata = psendbuf;
            send_pkt.msgid = 0x0201;
            comm_send_dirsend(&send_pkt);
            mem_free(psendbuf);
        }
    }
}

/*
********************************************************************************
* 注册回调函数
********************************************************************************
*/
static const FUNCENTRY_COMM_T s_functionentry[] = {
        0x8201, _handle_0x8201
};
static const INT8U s_funnum = sizeof(s_functionentry) / sizeof(s_functionentry[0]);

/*******************************************************************
** 函数名:      lamp_manager_init
** 函数描述:    lamp初始化
** 参数:        无
** 返回:        无
********************************************************************/
void lamp_manager_init(void)
{
    INT8U i, j;

    memset(&s_priv, 0, sizeof(Priv_t));
    memset((INT8U*)s_event, 0, sizeof(s_event));

    if (public_para_manager_check_valid_by_id(LAMP_EVENT_)) {
        SYS_DEBUG("LAMP_EVENT_ is valid\n");
        public_para_manager_read_by_id(LAMP_EVENT_, (INT8U*)s_event, sizeof(s_event));
    } else {
        for (i = 0; i < LAMP_NUM_MAX; i++) {
            for (j = 0; j < LAMP_EVENT_MAX; j++) {
                s_event[i][j].event = 0xff;
            }
        }
    }

    _update_lamp_status();
    s_lamptmr = os_timer_create(0, _lamp_manager_tmr);
    os_timer_start(s_lamptmr, 1);

    for (i = 0; i < s_funnum; i++) {
        comm_recv_register(s_functionentry[i].index, s_functionentry[i].entryproc);
    }
}

/*******************************************************************
** 函数名:      lamp_event_create
** 函数描述:    创建一个lamp event
** 参数:        [in] lamp    : 灯具编号
**              [in] event   : 控制事件指针
** 返回:        true or false
********************************************************************/
BOOLEAN lamp_event_create(Lamp_e lamp, Lamp_event_t *event)
{
    return_val_if_fail((lamp < LAMP_NUM_MAX) &&
        (event->event < LAMP_EVENT_MAX) && (event->dimming <= 100), FALSE);

    if (memcmp(&s_event[lamp][event->event], event, sizeof(Lamp_event_t)) == 0) {
        return TRUE;                                                           /* 控制事件一致，则退出，避免重新存储 */
    }
    
    memcpy(&s_event[lamp][event->event], event, sizeof(Lamp_event_t));
    public_para_manager_store_by_id(LAMP_EVENT_, (INT8U*)s_event, sizeof(s_event));
    _update_lamp_status();

    os_timer_start(s_lamptmr, 1);
    //SYS_DEBUG("lamp_event_create\n");
    return TRUE;
}

/*******************************************************************
** 函数名:      lamp_ctl
** 函数描述:    删除一个lamp event
** 参数:        [in] lamp    : 灯具编号
**              [in] event   : 控制事件
** 返回:        true or false
********************************************************************/
BOOLEAN lamp_event_delete(Lamp_e lamp, Lamp_envnt_e event)
{
    return_val_if_fail((lamp < LAMP_NUM_MAX) && (event < LAMP_EVENT_MAX), FALSE);

    if (s_event[lamp][event].event == 0xff) {
        return TRUE;
    }

    s_event[lamp][event].event = 0xff; 
    public_para_manager_store_by_id(LAMP_EVENT_, (INT8U*)s_event, sizeof(s_event));
    _update_lamp_status();
    
    os_timer_start(s_lamptmr, 1);
    //SYS_DEBUG("lamp_event_delete\n");
    return TRUE;
}

/*******************************************************************
** 函数名:      lamp_get_status
** 函数描述:    获取指定灯号当前的状态
** 参数:        [in] lamp    : 灯具编号
** 返回:        Lamp_tvent_t
********************************************************************/
Lamp_event_t* lamp_get_status(Lamp_e lamp)
{
    return_val_if_fail((lamp < LAMP_NUM_MAX), NULL);

    if (s_priv.lamp_st[lamp].prio_event == 0xff) {
        return NULL;
    } else {
        return &s_event[lamp][s_priv.lamp_st[lamp].prio_event];
    }
}

/*******************************************************************
** 函数名:      lamp_is_open
** 函数描述:    判断灯开/关状态
** 参数:        [in] lamp:
** 返回:        true or false
********************************************************************/
BOOLEAN lamp_is_open(Lamp_e lamp)
{
    return_val_if_fail((lamp < LAMP_NUM_MAX), NULL);
    
    if (s_priv.lamp_st[lamp].ctl == LAMP_OPEN) {
        return true;
    } else {
        return false;
    }
}

/*******************************************************************
** 函数名:      lamp_get_dimming
** 函数描述:    获取调光值
** 参数:        [in] lamp:
** 返回:        dimming
********************************************************************/
INT8U lamp_get_dimming(Lamp_e lamp)
{
    return_val_if_fail((lamp < LAMP_NUM_MAX), NULL);
    return s_priv.lamp_st[lamp].dimming;
}

