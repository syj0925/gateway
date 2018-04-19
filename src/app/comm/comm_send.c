/********************************************************************************
**
** 文件名:     comm_send.c
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

#define NUM_MEM              10
#define PERIOD_SEND          SECOND

/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
typedef struct {
    INT16U          flowseq;
    INT8U           ct_send;                    /* 重发次数 */
    INT8U           ct_time;                    /* 重发等待时间计数 */
    INT8U           flowtime;                   /* 重发等待时间 */
    Comm_pkt_send_t packet;
                                               /* 发送结果通知回调 */
    void           (*fp)(INT16U flowseq, Comm_pkt_send_t *packet, INT8U result);
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
static LIST_T s_sendlist, s_freelist;

extern Sys_base_info_t g_sys_base_info;

/*******************************************************************
** 函数名:     _get_flow_seq
** 函数描述:   得到数据帧流水号
** 参数:       无
** 返回:       数据帧流水号(强制从1开始, 0表示无效的发送流水号)
********************************************************************/
INT16U _get_flow_seq(void)
{
    static INT16U s_flowseq = 0xffff;

    if (s_flowseq == 0xffff) {
        s_flowseq = 1;
    } else {
        ++s_flowseq;
    }
    return s_flowseq;
}

/*******************************************************************
** 函数名:      comm_frame_send
** 函数描述:    链路帧数据发送
** 参数:        [in] flowseq : 流水号
**              [in] packet  : 发送数据包
** 返回:        true or false
********************************************************************/
static BOOLEAN _comm_frame_send(INT16U flowseq, Comm_pkt_send_t* packet)
{
    INT8U *pframebuf, *psendbuf, *ptr;
    INT16U framelen, sendlen;
    Stream_t wstrm;
    
    framelen  = packet->len + FRAMEHEAD_LEN + FRAMETAIL_LEN;
    pframebuf = (INT8U*)mem_malloc(framelen);
    
    if (pframebuf == NULL) {
        return false;
    } 

    stream_init(&wstrm, pframebuf, framelen);
    stream_write_half_word(&wstrm, packet->msgid);
    stream_write_half_word(&wstrm, packet->len);
    stream_write_half_word(&wstrm, g_sys_base_info.uid);
    stream_write_half_word(&wstrm, flowseq);
    stream_write_data(&wstrm, packet->pdata, packet->len);
    stream_write_byte(&wstrm, chksum_xor(stream_get_start_pointer(&wstrm), stream_get_len(&wstrm)));

    sendlen = 2*framelen + 2;
    psendbuf = (INT8U*)mem_malloc(sendlen);
    
    if (psendbuf == NULL) {
        mem_free(pframebuf);
        return false;
    } 

    sendlen = assemble_by_rules(psendbuf, pframebuf, framelen, (Asmrule_t *)&g_comm_rules);
    /* 调用发送函数 */
    ptr = psendbuf;

#if EN_ETHERNET > 0
    drv_ethernet_write(psendbuf, sendlen);
#endif

#if EN_ZIGBEE > 0
    drv_zigbee_write(psendbuf, sendlen);
#endif

#if EN_WIFI > 0
    drv_esp8266_write(psendbuf, sendlen);
#endif

    SYS_DEBUG("frame_send len[%d] ", sendlen);
    while (sendlen-- > 0) {
        SYS_DEBUG("%0.2x ", *ptr);
        
#if 0                                                                          /* 暂时作为TTS语音用 */
		while (USART_GetFlagStatus(EVAL_COM3, USART_FLAG_TC) == RESET);
		USART_SendData(EVAL_COM3, *ptr);
#endif /* if 0. 2017-8-14 14:32:50 syj */
        ptr++;
    }

    SYS_DEBUG("\n");

    mem_free(pframebuf);
    mem_free(psendbuf);
    
    return true;
}

/*******************************************************************
** 函数名:     _del_cell
** 函数描述:   删除节点
** 参数:       [in] cell:链表节点
** 参数:       [in] result:结果
** 返回:       无
********************************************************************/
static void _del_cell(CELL_T *cell, INT8U result)
{
    void (*fp)(INT16U flowseq, Comm_pkt_send_t *packet, INT8U result);
    
    fp = cell->fp;
    if (fp != 0) { 
        fp(cell->flowseq, &cell->packet, result);
    }
    mem_free(cell->packet.pdata);
    
    dlist_append_ele(&s_freelist, (LISTMEM *)cell);
}

/*******************************************************************
** 函数名:     _send_tmr_proc
** 函数描述:   扫描定时器
** 参数:       [in] pdata:定时器特征值
** 返回:       无
********************************************************************/
static void _send_tmr_proc(void *pdata)
{
    CELL_T *cell, *next;
    
    pdata = pdata;

    if (dlist_item(&s_sendlist) == 0) {
        os_timer_stop(s_sendtmr);
        return;
    }
    
    os_timer_start(s_sendtmr, PERIOD_SEND);
       
    cell = (CELL_T *)dlist_get_head(&s_sendlist);                              /* 扫描链表，检测重传时间是否超时 */
    for (;;) {
        if (cell == 0) break;
        if (++cell->ct_time > cell->flowtime) {                                /* 重传时间超时 */
            cell->ct_time = 0;
            if (--cell->ct_send == 0) {                                        /* 超过最大重传次数 */
                next = (CELL_T *)dlist_del_ele(&s_sendlist, (LISTMEM *)cell);
                _del_cell(cell, _OVERTIME);
                cell = next;
                continue;
            } else {
                _comm_frame_send(cell->flowseq, &cell->packet);
            }
        }
        cell = (CELL_T *)dlist_next_ele((LISTMEM *)cell);
    }
}

/*******************************************************************
** 函数名:     comm_send_init
** 函数描述:   发送模块初始化
** 参数:       无
** 返回:       无
********************************************************************/
void comm_send_init(void)
{
    dlist_init(&s_sendlist);

    dlist_mem_init(&s_freelist, (LISTMEM *)s_memory, sizeof(s_memory)/sizeof(s_memory[0]), sizeof(s_memory[0]));
    s_sendtmr = os_timer_create((void *)0, _send_tmr_proc);
}

/*******************************************************************
** 函数名:     comm_send_listsend
** 函数描述:   link数据链表发送
** 参数:       [in] packet : 数据指针
**             [in] ct_send: 发送次数
**             [in] ct_time: 重发等待时间，单位：秒
**             [in] fp     : 发送结果通知
** 返回:       返回流水号，0:表示挂接发送链表失败
********************************************************************/
INT16U comm_send_listsend(Comm_pkt_send_t *packet, INT8U ct_send, INT16U ct_time, void(*fp)(INT16U, Comm_pkt_send_t*, INT8U))
{
    CELL_T *cell;         
   
    return_val_if_fail((packet != NULL), false);

    if ((cell = (CELL_T *)dlist_del_head(&s_freelist)) != 0) {                 /* 申请链表节点 */
        memcpy(&cell->packet, packet, sizeof(Comm_pkt_send_t));
        cell->packet.pdata = (INT8U*)mem_malloc(cell->packet.len);
        
        if (cell->packet.pdata == NULL) {
            dlist_append_ele(&s_freelist, (LISTMEM *)cell);
            return 0;
        } else {
            memcpy(cell->packet.pdata, packet->pdata, packet->len);
        }
        
        cell->flowseq  = _get_flow_seq();
        cell->ct_send  = ct_send;
        cell->flowtime = ct_time;                                              /* 等待重传时间 */
        cell->ct_time  = 0;
        cell->fp       = fp;

        dlist_append_ele(&s_sendlist, (LISTMEM *)cell);                        /* 将新增节点放入就绪链表 */
        
        if (!os_timer_is_run(s_sendtmr)) {
            os_timer_start(s_sendtmr, PERIOD_SEND);
        }

        _comm_frame_send(cell->flowseq, &cell->packet);
        return cell->flowseq;
    } else {
        return 0;
    }
}

/*******************************************************************
** 函数名:     comm_send_dirsend
** 函数描述:   直接发送，不挂到链表发送
** 参数:       [in] packet : 数据包
** 返回:       发送成功返回true，失败返回false
********************************************************************/
BOOLEAN comm_send_dirsend(Comm_pkt_send_t *packet)
{
    return_val_if_fail((packet != NULL), false);

    _comm_frame_send(_get_flow_seq(), packet);

    return true;
}

/*******************************************************************
** 函数名:     comm_send_confirm
** 函数描述:   应答确认
** 参数:       [in] flowseq : 流水号
**             [in] result  : 结果，成功 or 失败
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN comm_send_confirm(INT16U flowseq, INT8U result)
{
    CELL_T  *cell;
    
    result = result;
    
    cell = (CELL_T *)dlist_get_head(&s_sendlist);
    for (;;) {                                                                 /* 查找等待链表 */
        if (cell == 0) break;
        if (cell->flowseq == flowseq) {
            dlist_del_ele(&s_sendlist, (LISTMEM *)cell);
            _del_cell(cell, result);
            return true;
        } else {
            cell = (CELL_T *)dlist_next_ele((LISTMEM *)cell);                  /* 扫描下个节点 */
        }
    }
    
    return false;        
}

