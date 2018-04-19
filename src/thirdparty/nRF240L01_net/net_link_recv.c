/********************************************************************************
**
** 文件名:     net_link_recv.c
** 版权所有:   (c) 2013-2014 
** 文件描述:   link 接收模块
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**|    日期    |  作者  |  修改记录
**===============================================================================
**| 2014/10/16 | 苏友江 |  创建该文件
********************************************************************************/
#include "sys_includes.h"
#include "nrf24l01.h"
#include "net_typedef.h"
#include "net_link_node.h"

//#include "led_ctl.h"

#if DBG_SYSTEM > 0
#define SYS_DEBUG          OS_DEBUG
#else
#define SYS_DEBUG(...)     do{}while(0)
#endif

/*
********************************************************************************
* 宏定义
********************************************************************************
*/
#define MAX_REG              8
#define PERIOD_SCAN          1*MILTICK

/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
typedef struct protocolreg {
    struct protocolreg  *next;
    INT8U type;
    void  (*handler)(INT8U type, PACKET_BUF_T *packet);
} PROTOCOL_REG_T;


/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/
static INT8U s_scantmr;
static PROTOCOL_REG_T s_reg_tbl[MAX_REG];
static PROTOCOL_REG_T *s_usedlist, *s_freelist;
static PACKET_BUF_T   s_packet_buf;


/*******************************************************************
** 函数名:     _hdl_recv_data
** 函数描述:   协议处理
** 参数:       [in] pframe:
** 返回:       无
********************************************************************/
static void _hdl_recv_data(PACKET_BUF_T *packet_buf)
{
    INT8U type;
    LINK_HEAD_T     *pframe;
    PROTOCOL_REG_T  *curptr;
    Net_link_node_t *node = NULL;

    pframe = (LINK_HEAD_T*)packet_buf->buf;
    type   = pframe->type;

    SYS_DEBUG("<nrf24l01 recv: type:0x%0.2x, addr:%0.2x, signal:%d>\r\n", type, pframe->srcaddr[0], nrf24l01_get_signal());

    if (type == 0x01) {                                                        /* 入网请求 */
        node = NULL;       
    } else {
        node = net_link_node_update(pframe->srcaddr);

        if (node == NULL) {                                                    /* 节点不存在，不处理数据 */
            return;
        }
    }

    packet_buf->node  = node;
    packet_buf->pdata = pframe->data;
    packet_buf->len   = PLOAD_WIDTH - LINK_HEAD_LEN;

    curptr = s_usedlist;
    while (curptr != 0) {
        if (curptr->type == type) {                                            /* 搜索对应类型 */
            return_if_fail(curptr->handler != 0);                              /* 注册的处理函数不能为空 */
            curptr->handler(type, packet_buf);
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
    (void)pdata;

	if ((nrf24l01_irq() != 0) || (nrf24l01_packet_recv(s_packet_buf.buf, PLOAD_WIDTH) == 0)) {
        return;
	}
    //api_led_ctl(PIN_LED_RED, TURN_ON, 1);                                      /* 收到数据闪烁led */

    _hdl_recv_data(&s_packet_buf);
}

/*******************************************************************
** 函数名:     net_link_recv_init
** 函数描述:   link接收模块初始化
** 参数:       无
** 返回:       无
********************************************************************/
void net_link_recv_init(void)
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
    os_timer_start(s_scantmr, 1);
}

/*******************************************************************
** 函数名:     net_link_register
** 函数描述:   link接收处理注册
** 参数:       [in] type:   协议类型
**             [in] handler:协议处理函数
** 返回:       注册成功返回true，注册失败返回false
********************************************************************/
BOOLEAN net_link_register(INT8U type, void (*handler)(INT8U type, PACKET_BUF_T *packet))
{
    PROTOCOL_REG_T *curptr;

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

