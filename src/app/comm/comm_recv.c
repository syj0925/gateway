/********************************************************************************
**
** 文件名:     comm_recv.c
** 版权所有:   (c) 2013-2015 
** 文件描述:   通信接收模块
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**|    日期    |  作者  |  修改记录
**===============================================================================
**| 2015/09/08 | 苏友江 |  创建该文件
********************************************************************************/
#define GLOBALS_COMM_RECV     1
#include "bsp.h"
#include "sys_includes.h"

#if DBG_COMM > 0
#define SYS_DEBUG          OS_DEBUG
#else
#define SYS_DEBUG(...)     do{}while(0)
#endif
/*
********************************************************************************
* 宏定义
********************************************************************************
*/
#define MAX_REG              20
#define PERIOD_SCAN          1*MILTICK
#define SIZE_HDL             256

/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
typedef struct protocolreg {
    struct protocolreg  *next;
    INT16U type;
    void  (*handler)(Comm_pkt_recv_t *packet);
} Protocol_reg_t;

typedef struct {
    INT16U rlen;
    INT8U  rbuf[SIZE_HDL];
} Priv_t;
/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/
static INT8U s_scantmr;
static Protocol_reg_t s_reg_tbl[MAX_REG];
static Protocol_reg_t *s_usedlist, *s_freelist;
static Comm_pkt_recv_t  s_packet_recv;
static Priv_t s_priv;
static INT16U s_last_flowseq = 0;
static INT8U s_recv_buf[100];
static RoundBuf_t s_roundbuf = {0};

extern Sys_base_info_t g_sys_base_info;


void USART3_IRQHandler(void)
{
	if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET) {
		roundbuf_write_byte(&s_roundbuf, (USART_ReceiveData(USART3) & 0xFF));
	}
}

static BOOLEAN _check_is_own_group(INT8U group)
{
    INT8U i;
    
    for (i = 0; i < GROUP_NUM; i++) {
        if (g_sys_base_info.group[i] == group) {
            return TRUE;
        }
    }
    return FALSE;
}

/*******************************************************************
** 函数名:     _hdl_recv_data
** 函数描述:   协议处理
** 参数:       [in] pdata:
**             [in] datalen:
** 返回:       无
********************************************************************/
static void _hdl_recv_data(INT8U *pdata, INT16U datalen)
{
    INT8U mode, exheadlen = 0;
    INT16U type, msgatr, uid;
    Protocol_reg_t *curptr;
    Comm_frame_t   *pframe = (Comm_frame_t*)pdata;

    SYS_DEBUG("_hdl_recv_data len:%d\n", datalen);

    type   = (pframe->msgid[0]  << 8) + pframe->msgid[1];
    msgatr = (pframe->msgatr[0] << 8) + pframe->msgatr[1];
    uid    = (pframe->uid[0]    << 8) + pframe->uid[1];
    s_packet_recv.flowseq = (pframe->flowseq[0] << 8) + pframe->flowseq[1];
    
    mode = msgatr >> 14;
    s_packet_recv.ack = 1;
    if (mode == 0x00) {                                                        /* 单播 */
        if ((g_sys_base_info.uid&0xffff) != uid) {
            return;
        }
    } else if (mode == 0x01) {                                                 /* 组播 */
        if (!_check_is_own_group(pdata[FRAMEHEAD_LEN])) {
            return;
        }
        exheadlen += 1;                                                        /* 组号占一个byte */
    }
    
    if ((g_sys_base_info.uid&0xffff) != uid) {
        s_packet_recv.ack = 0;

        if (s_last_flowseq == 0) {
            s_last_flowseq = s_packet_recv.flowseq;
        } else {
            if (s_last_flowseq == s_packet_recv.flowseq) {                     /*  流水号一致，证明之前已经收到过 */
                return;
            } else {
                s_last_flowseq = s_packet_recv.flowseq;
            }
        }
    }

    if (msgatr & 0x2000) {
        exheadlen += 4;                                                        /* 分包信息包含4个bytes */
    }

    s_packet_recv.len = (msgatr & 0x03FF);
    if (s_packet_recv.len != datalen - FRAMEHEAD_LEN - FRAMETAIL_LEN - exheadlen) {
        return;
    }

#if 0
    if (chksum_xor(pdata, datalen - 1) != pdata[datalen - 1]) {
        return;
    }
#endif /* if 0. 2015-9-14 16:11:14 syj */

    s_packet_recv.pdata   = &pdata[FRAMEHEAD_LEN + exheadlen];

    curptr = s_usedlist;
    while (curptr != 0) {
        if (curptr->type == type) {                                            /* 搜索对应类型 */
            return_if_fail(curptr->handler != 0);                              /* 注册的处理函数不能为空 */
            curptr->handler(&s_packet_recv);
            break;
        } else {
            curptr = curptr->next;
        }
    }
}

/*******************************************************************
** 函数名:     ScanTmrProc
** 函数描述:   扫描定时器
** 参数:       [in] pdata:定时器特征值
** 返回:       无
********************************************************************/
static void scan_tmr_proc(void *pdata)
{
    INT32S len;
    INT8U ch, tempbuf[30];
    INT16S recv;

    len = sizeof(tempbuf);
#if EN_ETHERNET > 0
    if ((len = drv_ethernet_read(tempbuf, len)) > 0) {
        roundbuf_write_block(&s_roundbuf, tempbuf, len);
    }
#endif    

#if EN_ZIGBEE > 0
    if ((len = drv_zigbee_read(tempbuf, len)) > 0) {
        roundbuf_write_block(&s_roundbuf, tempbuf, len);
    }
#endif

#if EN_WIFI > 0
    if ((len = drv_esp8266_read(tempbuf, len)) > 0) {
        roundbuf_write_block(&s_roundbuf, tempbuf, len);
    }
#endif   

    for (;;) {
        if ((recv = roundbuf_read_byte(&s_roundbuf)) == -1) {
            break;
        }
        ch = recv;
        SYS_DEBUG("%0.2x ", ch);
        if (s_priv.rlen == 0) {
            if (ch == g_comm_rules.c_flags) {                                  /* 检测协议头 */
                s_priv.rbuf[0] = 0;
                s_priv.rlen++;
            }
        } else {
            if (ch == g_comm_rules.c_flags) {
                if (s_priv.rlen > 1) {
                    s_priv.rlen = deassemble_by_rules(s_priv.rbuf, &s_priv.rbuf[1], s_priv.rlen - 1, (Asmrule_t *)&g_comm_rules);
                    if (s_priv.rlen >= 2) {
                        _hdl_recv_data(s_priv.rbuf, s_priv.rlen);
                    }
                    s_priv.rlen = 0;
                }
            } else {
                if (s_priv.rlen >= SIZE_HDL) {
                    s_priv.rlen = 0;
                } else {
                    s_priv.rbuf[s_priv.rlen++] = ch;
                }
            }
        }
    }
}

/*******************************************************************
** 函数名:     comm_recv_init
** 函数描述:   接收模块初始化
** 参数:       无
** 返回:       无
********************************************************************/
void comm_recv_init(void)
{
    INT8U i;
    
    for (i = 0; i < MAX_REG - 1; i++) {
        s_reg_tbl[i].next = &s_reg_tbl[i + 1];
        s_reg_tbl[i].type = 0;
        s_reg_tbl[i].handler = 0;
    }
    s_reg_tbl[i].next      = 0;
    s_reg_tbl[i].type      = 0;
    s_reg_tbl[i].handler   = 0;
    
    s_freelist             = &s_reg_tbl[0];
    s_usedlist             = 0;
    
    s_scantmr = os_timer_create((void *)0, scan_tmr_proc);
    os_timer_start(s_scantmr, PERIOD_SCAN);

    roundbuf_init(&s_roundbuf, s_recv_buf, sizeof(s_recv_buf), NULL);  
    usart_rxit_enable(COM3);
}

/*******************************************************************
** 函数名:     comm_recv_register
** 函数描述:   接收处理注册
** 参数:       [in] type   : 协议类型
**             [in] handler: 协议处理函数
** 返回:       注册成功返回true，注册失败返回false
********************************************************************/
BOOLEAN comm_recv_register(INT16U type, void (*handler)(Comm_pkt_recv_t *packet))
{
    Protocol_reg_t *curptr;

    curptr = s_usedlist;
    while (curptr != 0) {                                                      /* 不能重复注册 */
        return_val_if_fail(curptr->type != type, FALSE);
        curptr = curptr->next;
    }

    return_val_if_fail(handler != 0, FALSE);                                   /* 注册的处理函数不能为空 */
    return_val_if_fail(s_freelist != 0, FALSE);                                /* 注册表已满则出错 */

    curptr = s_freelist;                                                       /* 新注册 */
    if (curptr != 0) {
        s_freelist        = curptr->next;
        curptr->next      = s_usedlist;
        s_usedlist        = curptr;
        curptr->type      = type;
        curptr->handler   = handler;                                           /* 处理函数 */
        return true;
    } else {
        return false;
    }
}

