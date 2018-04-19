/********************************************************************************
**
** 文件名:     net_link_send.c
** 版权所有:   (c) 2013-2014 
** 文件描述:   协议封装发送，发送链表管理
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**|    日期    |  作者  |  修改记录
**===============================================================================
**| 2014/10/18 | 苏友江 |  创建该文件
********************************************************************************/
#include "sys_includes.h"
#include "nrf24l01.h"
#include "net_typedef.h"
#include "net_link.h"
#include "net_link_node.h"
#include "net_link_send.h"

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

#define NUM_MEM              10
#define PERIOD_SEND          2*SECOND
#define OVERTIME             7
/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
typedef struct {
    INT8U        type;                       /* 命令类型 */
    INT8U        ct_send;                    /* 重发次数 */
    INT16U       ct_time;                    /* 重发等待时间计数 */
    INT16U       flowtime;                   /* 重发等待时间 */
    PACKET_BUF_T packet;
    void         (*fp)(INT8U result);        /* 发送结果通知回调 */
} CELL_T;
/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/
static struct {
    NODE   reserve;
    CELL_T cell;
} s_memory[NUM_MEM];

static INT8U s_sendtmr;
static LIST_T s_sendlist, s_waitlist, s_freelist;

/*******************************************************************
** 函数名:      net_link_frame_send
** 函数描述:    链路帧数据发送
** 参数:        [in] type : 帧类型
**              [in] pbuf : 发送数据包
** 返回:        true or false
********************************************************************/
static BOOLEAN net_link_frame_send(INT8U type, PACKET_BUF_T* pbuf)
{
    INT8U ret;
    INT8U offset;
    Net_link_node_t *node;

    node = pbuf->node;
    nrf24l01_mode_switch(STANDBY);

    memcpy(pbuf->buf, node->addr, ADR_WIDTH);                                  /* 目的地址 */

    nrf24l01_dest_addr_set(node->routeaddr, ADR_WIDTH);                        /* 设置发送地址-路由地址 */
    
    offset = ADR_WIDTH;
    memcpy(&pbuf->buf[offset], net_link_get_local_addr(), ADR_WIDTH);
    offset += ADR_WIDTH;
    pbuf->buf[offset] = type;

    ret = nrf24l01_packet_send(pbuf->buf, PLOAD_WIDTH);  
    nrf24l01_mode_switch(RX);
    //api_led_ctl(PIN_LED_GREEN, TURN_ON, 1);                                    /* 收到数据闪烁led */
    SYS_DEBUG("<nrf24l01 send: type:0x%0.2x, addr:%0.2x, ret:%d>\n", type, node->addr[0], ret);

    return true;
}

/*******************************************************************
** 函数名:     del_cell
** 函数描述:   删除节点
** 参数:       [in] cell:链表节点
** 参数:       [in] result:结果
** 返回:       无
********************************************************************/
static void del_cell(CELL_T *cell, INT8U result)
{
    void (*fp)(INT8U result);
    
    fp = cell->fp;
    dlist_append_ele(&s_freelist, (LISTMEM *)cell);            
    if (fp != 0) { 
        fp(result);
    }
}

/*******************************************************************
** 函数名:     send_tmr_proc
** 函数描述:   扫描定时器
** 参数:       [in] pdata:定时器特征值
** 返回:       无
********************************************************************/
static void send_tmr_proc(void *pdata)
{
    CELL_T *cell, *next;
    
    pdata = pdata;

#if SEND_MODE == PASSIVE_SEND
    if (dlist_item(&s_sendlist) + dlist_item(&s_waitlist) == 0) {
        os_timer_stop(s_sendtmr);
        return;
    }
#else
    if (dlist_item(&s_sendlist) == 0) {
        os_timer_stop(s_sendtmr);
        return;
    }
#endif
    
    os_timer_start(s_sendtmr, PERIOD_SEND);
       
    cell = (CELL_T *)dlist_get_head(&s_sendlist);                              /* 扫描链表，检测重传时间是否超时 */
    for (;;) {
        if (cell == 0) break;
        if (++cell->ct_time > cell->flowtime) {                                /* 重传时间超时 */
            cell->ct_time = 0;
            if (--cell->ct_send == 0) {                                        /* 超过最大重传次数 */
                next = (CELL_T *)dlist_del_ele(&s_sendlist,(LISTMEM *)cell);
                del_cell(cell, _OVERTIME);
                cell = next;
                continue;
            } else {
                net_link_send_dirsend(cell->type, &cell->packet);
            }
        }
        cell = (CELL_T *)dlist_next_ele((LISTMEM *)cell);
    }
    
#if SEND_MODE == PASSIVE_SEND
    cell = (CELL_T *)dlist_get_head(&s_waitlist);                              /* 扫描链表，检测重传时间是否超时 */
    for (;;) {
        if (cell == 0) break;
        
        if ((cell->ct_send == 0) || (++cell->ct_time > cell->flowtime)) {      /* 重传时间超时 */
            cell->ct_time = 0;
            next = (CELL_T *)dlist_del_ele(&s_waitlist, (LISTMEM *)cell);
            SYS_DEBUG("<Over time>\n");
            del_cell(cell, _OVERTIME);
            cell = next;
            continue;
        }
        cell = (CELL_T *)dlist_next_ele((LISTMEM *)cell);
    }
#endif
}

/*******************************************************************
** 函数名:     net_link_send_init
** 函数描述:   link发送模块初始化
** 参数:       无
** 返回:       无
********************************************************************/
void net_link_send_init(void)
{
    dlist_init(&s_sendlist);
#if SEND_MODE == PASSIVE_SEND    
    dlist_init(&s_waitlist);
#endif
    dlist_mem_init(&s_freelist, (LISTMEM *)s_memory, sizeof(s_memory)/sizeof(s_memory[0]), sizeof(s_memory[0]));
    s_sendtmr = os_timer_create((void *)0, send_tmr_proc);
}

/*******************************************************************
** 函数名:     net_link_send_listsend
** 函数描述:   link数据链表发送
** 参数:       [in] type   : 协议类型
**             [in] packet : 数据指针
**             [in] ct_send: 发送次数
**             [in] ct_time: 重发等待时间，单位：秒
**             [in] fp     : 发送结果通知
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN net_link_send_listsend(INT8U type, PACKET_BUF_T *packet, INT8U ct_send, INT16U ct_time, void(*fp)(INT8U))
{
    CELL_T *cell;         
    Net_link_node_t *node;
   
    return_val_if_fail((packet != NULL) && (packet->node != NULL), false);

#if SEND_MODE == PASSIVE_SEND
    node = packet->node;
    if ((node->type == ENDDEVICE) && (strncmp((char*)node->addr, (char*)node->routeaddr, ADR_WIDTH) == 0)) {
        if ((cell = (CELL_T *)dlist_del_head(&s_freelist)) != 0) {             /* 申请链表节点 */
            memcpy(&cell->packet, packet, sizeof(PACKET_BUF_T));
            
            cell->type     = type;
            cell->ct_send  = ct_send;
            cell->flowtime = OVERTIME;                                         /* 等待重传时间 */
            cell->ct_time  = 0;
            cell->fp       = fp;

            dlist_append_ele(&s_waitlist, (LISTMEM *)cell);                    /* 将新增节点放入就绪链表 */

            if (!os_timer_is_run(s_sendtmr)) {
                os_timer_start(s_sendtmr, PERIOD_SEND);
            }
            
            return true;
        } else {
            return false;
        }
    }
#endif

    if ((cell = (CELL_T *)dlist_del_head(&s_freelist)) != 0) {                 /* 申请链表节点 */
        memcpy(&cell->packet, packet, sizeof(PACKET_BUF_T));
        
        cell->type     = type;
        cell->ct_send  = ct_send;
        cell->flowtime = ct_time;                                              /* 等待重传时间 */
        cell->ct_time  = 0;
        cell->fp       = fp;

        dlist_append_ele(&s_sendlist, (LISTMEM *)cell);                        /* 将新增节点放入就绪链表 */
        
        if (!os_timer_is_run(s_sendtmr)) {
            os_timer_start(s_sendtmr, PERIOD_SEND);
        }

        net_link_send_dirsend(type, &cell->packet);
        return true;
    } else {
        return false;
    }
}

/*******************************************************************
** 函数名:     net_link_send_dirsend
** 函数描述:   直接发送，不挂到链表发送
** 参数:       [in] type  : 协议类型
**             [in] packet: 数据包
** 返回:       发送成功返回true，失败返回false
********************************************************************/
BOOLEAN net_link_send_dirsend(INT8U type, PACKET_BUF_T *packet)
{
    return_val_if_fail((packet != NULL) && (packet->node != NULL), false);

    net_link_frame_send(type, packet);
    return true;
}

/*******************************************************************
** 函数名:     net_link_send_confirm
** 函数描述:   应答确认
** 参数:       [in] type: 协议类型
**             [in] result: 结果
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN net_link_send_confirm(INT8U type, PACKET_BUF_T *packet, INT8U result)
{
    CELL_T  *cell;
    
    result = result;
    
    cell = (CELL_T *)dlist_get_head(&s_sendlist);
    for (;;) {                                                                 /* 查找发送链表 */
        if (cell == 0) break;
        if ((cell->type == type) &&                                            /* 查找到匹配的节点 */
            (strncmp((char*)cell->packet.node->addr, (char*)packet->node->addr, ADR_WIDTH) == 0)) {
            dlist_del_ele(&s_sendlist, (LISTMEM *)cell);
            del_cell(cell, _SUCCESS);
            return true;
        } else {
            cell = (CELL_T *)dlist_next_ele((LISTMEM *)cell);                  /* 扫描下个节点 */
        }
    }

#if SEND_MODE == PASSIVE_SEND
    cell = (CELL_T *)dlist_get_head(&s_waitlist);
    for (;;) {                                                                 /* 查找等待链表 */
        if (cell == 0) break;
        if ((cell->type == type) &&                                            /* 查找到匹配的节点 */
            (strncmp((char*)cell->packet.node->addr, (char*)packet->node->addr, ADR_WIDTH) == 0)) {
            SYS_DEBUG("<send_confirm:addr:%x, type:%0.2x>\n", cell->packet.node->addr[0], type);
            
            dlist_del_ele(&s_waitlist, (LISTMEM *)cell);
            del_cell(cell, _SUCCESS);
            return true;
        } else {
            cell = (CELL_T *)dlist_next_ele((LISTMEM *)cell);                  /* 扫描下个节点 */
        }
    }
#endif
    return false;        
}

#if SEND_MODE == PASSIVE_SEND
/*******************************************************************
** 函数名:      net_link_send_inform
** 函数描述:    通知收到数据
** 参数:        [in] node:
** 返回:        ture or false
********************************************************************/
BOOLEAN net_link_send_inform(Net_link_node_t *node)
{
    CELL_T *cell/*, *next*/;
        
    if (dlist_item(&s_waitlist) == 0) {
        return false;
    }

    cell = (CELL_T *)dlist_get_head(&s_waitlist);                              /* 扫描链表，检测重传时间是否超时 */
    for (;;) {
        if (cell == 0) break;
        if (strncmp((char*)cell->packet.node->addr, (char*)node->addr, ADR_WIDTH) == 0) {
            SYS_DEBUG("<send_inform:addr:%x>\n", node->addr[0]);
            net_link_send_dirsend(cell->type, &cell->packet);
            cell->ct_time = 0;
            --cell->ct_send;
            return true;
        }
        cell = (CELL_T *)dlist_next_ele((LISTMEM *)cell);
    }

    return false;
}
#endif

