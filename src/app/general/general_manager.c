/********************************************************************************
**
** 文件名:     general_manager.c
** 版权所有:   (c) 2013-2015 
** 文件描述:   通用业务管理
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**|    日期    |  作者  |  修改记录
**===============================================================================
**| 2015/10/01 | 苏友江 |  创建该文件
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




/*
********************************************************************************
* 结构定义
********************************************************************************
*/




#if 0
typedef struct {

} Priv_t;
#endif /* if 0. 2015-10-1 22:36:05 suyoujiang */

/*
********************************************************************************
* 静态变量
********************************************************************************
*/
extern Sys_run_info_t g_sys_run_info;

/*******************************************************************
** 函数名:      _handle_0x8001
** 函数描述:    平台通用应答
** 参数:        [in] packet:
** 返回:        none
********************************************************************/
static void _handle_0x8001(Comm_pkt_recv_t *packet)
{
    Stream_t rstrm;
    INT8U result;
    INT16U ackflowseq, acktypeid;    	

    stream_init(&rstrm, packet->pdata, packet->len);
    ackflowseq = stream_read_half_word(&rstrm);
    acktypeid  = stream_read_half_word(&rstrm);
    result     = stream_read_byte(&rstrm);

    comm_send_confirm(ackflowseq, result);
    
#if 1
    switch (acktypeid) {
        case 0: 
            if (result == 0) {
 
            } else {
 
            }     

        default:
            break;
    }
#endif /* if 0. 2015-10-1 21:20:26 suyoujiang */
}

/*******************************************************************
** 函数名:      _handle_0x8202
** 函数描述:    状态查询
** 参数:        [in] packet:
** 返回:        none
********************************************************************/
static void _handle_0x8202(Comm_pkt_recv_t *packet)
{
    if (packet->ack) {
        INT8U *psendbuf;
        Stream_t wstrm;
        Comm_pkt_send_t send_pkt;

        psendbuf = comm_protocol_asm_status(&wstrm);
        if (psendbuf != NULL) {
            send_pkt.len   = stream_get_len(&wstrm);
            send_pkt.pdata = psendbuf;
            send_pkt.msgid = 0x0202;
            comm_send_dirsend(&send_pkt);
            mem_free(psendbuf);
        }
    }
}

/*******************************************************************
** 函数名:      _handle_0x8204
** 函数描述:    电能查询
** 参数:        [in] packet:
** 返回:        none
********************************************************************/
static void _handle_0x8204(Comm_pkt_recv_t *packet)
{
    INT8U i, sendbuf[4+4+4+1+1+LAMP_NUM_MAX*20];
    Comm_pkt_send_t send_pkt;

    if (packet->ack) {
        Stream_t wstrm;
        
        stream_init(&wstrm, sendbuf, sizeof(sendbuf));
        stream_write_long(&wstrm, *((INT32U*)&g_power.v_eff_avr));
        stream_write_long(&wstrm, *((INT32U*)&g_power.i_leakage_eff_avr));
        stream_write_long(&wstrm, *((INT32U*)&g_power.v_leakage_eff_avr));
        stream_write_byte(&wstrm, g_power.fr);
        stream_write_byte(&wstrm, LAMP_NUM_MAX);

        for (i = 0; i < LAMP_NUM_MAX; i++) {
            stream_write_long(&wstrm, *((INT32U*)&g_power.lamp[i].i_eff_avr));
            stream_write_long(&wstrm, *((INT32U*)&g_power.lamp[i].p_avr));
            stream_write_long(&wstrm, *((INT32U*)&g_power.lamp[i].s_avr));
            stream_write_long(&wstrm, *((INT32U*)&g_power.lamp[i].ws));
            stream_write_long(&wstrm, *((INT32U*)&g_power.lamp[i].pf));
        }
        
        send_pkt.len   = stream_get_len(&wstrm);
        send_pkt.pdata = sendbuf;
        send_pkt.msgid = 0x0204;
        comm_send_dirsend(&send_pkt);
    }
}

/*******************************************************************
** 函数名:      _handle_0x8205
** 函数描述:    属性查询
** 参数:        [in] packet:
** 返回:        none
********************************************************************/
static void _handle_0x8205(Comm_pkt_recv_t *packet)
{
    char *p_ver;
    INT8U sendbuf[3+2*15+12];                                                  /* 15指的是软件和硬件版本不行超过此限制长度 */
    Comm_pkt_send_t send_pkt;
    
    if (packet->ack) {
        Stream_t wstrm;
        
        stream_init(&wstrm, sendbuf, sizeof(sendbuf));
        stream_write_byte(&wstrm, DEVICE_TYPE);                                /* 终端类型 */
        p_ver = HARDWARE_VERSION_STR;
        stream_write_byte(&wstrm, strlen(p_ver)); 
        stream_write_string(&wstrm, p_ver);
        
        p_ver = SOFTWARE_VERSION_STR;
        stream_write_byte(&wstrm, strlen(p_ver)); 
        stream_write_string(&wstrm, p_ver);        

        stream_write_long(&wstrm, g_sys_run_info.restartcnt);
        stream_write_long(&wstrm, g_sys_run_info.wdgcnt);
        stream_write_long(&wstrm, g_systime_sec);

        send_pkt.len   = stream_get_len(&wstrm);
        send_pkt.pdata = sendbuf;
        send_pkt.msgid = 0x0205;
        comm_send_dirsend(&send_pkt);
    }
}

/*******************************************************************
** 函数名:      _handle_0x8208
** 函数描述:    终端控制命令
** 参数:        [in] packet:
** 返回:        none
********************************************************************/
static void _handle_0x8208(Comm_pkt_recv_t *packet)
{
    Stream_t rstrm;
    INT8U result = 0, ctlcmd;

    stream_init(&rstrm, packet->pdata, packet->len);
    ctlcmd = stream_read_byte(&rstrm);

    switch (ctlcmd) {
        case 1:
            os_msg_post(TSK_SYS_MSG, MSG_PP_RESET, 0, 0, NULL);
            break;
        case 2:
            os_msg_post(TSK_SYS_MSG, MSG_PP_RESTOREFACTORY, 0, 0, NULL);
            break;            
        case 3:
            
            break;
        default:
            break;
    }

    if (packet->ack) {
        comm_protocol_common_ack(packet->flowseq, 0x8208, result);
    }
}

/*
********************************************************************************
* 注册回调函数
********************************************************************************
*/
static const FUNCENTRY_COMM_T s_functionentry[] = {
        0x8001, _handle_0x8001      /* 平台通用应答 */
       ,0x8202, _handle_0x8202      /* 终端状态查询 */
       ,0x8204, _handle_0x8204      /* 终端电能查询 */
       ,0x8205, _handle_0x8205      /* 终端属性查询 */
       ,0x8208, _handle_0x8208      /* 终端控制命令 */
};
static const INT8U s_funnum = sizeof(s_functionentry) / sizeof(s_functionentry[0]);

/*******************************************************************
** 函数名:      general_manager_init
** 函数描述:    通用业务管理初始化
** 参数:        无
** 返回:        无
********************************************************************/
void general_manager_init(void)
{
    INT8U i;

#if 0
    memset(&s_priv, 0, sizeof(Priv_t));
    s_lamptmr = os_timer_create(0, _general_manager_tmr);
    os_timer_start(s_lamptmr, 1);
#endif /* if 0. 2015-10-1 20:39:14 suyoujiang */

    for (i = 0; i < s_funnum; i++) {
        comm_recv_register(s_functionentry[i].index, s_functionentry[i].entryproc);
    }
}

/*******************************************************************
** 函数名:     OS_MsgReset
** 函数描述:   复位操作
** 参数:       [in]  lpara:
** 返回:       无
********************************************************************/
void OS_MsgReset(INT32U lpara, INT32U hpara, void *p)
{
    energy_measure_power_store();

    public_para_manager_reset_inform(0, NULL, 0, 0);
    __set_PRIMASK(1);
    NVIC_SystemReset();
}

/*******************************************************************
** 函数名:     OS_MsgRestoreFactory
** 函数描述:   恢复出厂设置
** 参数:       [in]  lpara:
** 返回:       无
********************************************************************/
void OS_MsgRestoreFactory(INT32U lpara, INT32U hpara, void *p)
{
    public_para_manager_del_all_and_rst();
    __set_PRIMASK(1);
    NVIC_SystemReset();
}

