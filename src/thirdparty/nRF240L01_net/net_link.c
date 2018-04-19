/********************************************************************************
**
** 文件名:     net_link.c
** 版权所有:   (c) 2013-2015 
** 文件描述:   网络链路层
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**|    日期    |  作者  |  修改记录
**===============================================================================
**| 2015/07/04 | 苏友江 |  创建该文件
********************************************************************************/
#include "sys_includes.h"
#include "nrf24l01.h"
#include "net_typedef.h"
#include "net_link_node.h"
#include "net_link_recv.h"
#include "net_link_send.h"

#if DBG_SYSTEM > 0
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



/*
********************************************************************************
* 静态变量
********************************************************************************
*/
static INT8U s_local_addr[ADR_WIDTH] = {
    0x99,           /* 本机地址, 根据现场需要可修改(避免不同主节点地址一样) */
};

static const INT8U c_leve1_addr[ADR_WIDTH] = {
    0x00            /* 一级入网请求地址，固定不能修改 */
};

/*******************************************************************
** 函数名:      _ack_0x01
** 函数描述:    入网请求应答
** 参数:        [in] node  : 节点
**              [in] result: 应答结果
** 返回:        无
********************************************************************/
static void _ack_0x01(PACKET_BUF_T *packet, INT8U result)
{
    Stream_t wstrm;

    stream_init(&wstrm, packet->pdata, PLOAD_WIDTH-LINK_HEAD_LEN);
    stream_write_byte(&wstrm, result);
    net_link_send_dirsend(0x81, packet);
}

/*******************************************************************
** 函数名:      _ack_0x02
** 函数描述:    链路请求应答
** 参数:        [in] node  : 节点
**              [in] result: 应答结果
** 返回:        无
********************************************************************/
static void _ack_0x02(PACKET_BUF_T *packet, INT8U result)
{
    Stream_t wstrm;

    stream_init(&wstrm, packet->pdata, PLOAD_WIDTH-LINK_HEAD_LEN);
    stream_write_byte(&wstrm, result);
    net_link_send_dirsend(0x82, packet);
}

/*******************************************************************
** 函数名:     _hdl_0x01
** 函数描述:   入网请求协议 
** 参数:       [in]cmd    : 命令编码
**             [in]packet : 数据指针
** 返回:       无
********************************************************************/
static void _hdl_0x01(INT8U cmd, PACKET_BUF_T *packet)
{
    BOOLEAN isnew;
    Stream_t rstrm;
    Net_link_node_t *node;
    INT8U flowseq, srcaddr[ADR_WIDTH], routeaddr[ADR_WIDTH];
    
    stream_init(&rstrm, packet->buf, PLOAD_WIDTH);
    stream_move_pointer(&rstrm, ADR_WIDTH);
    stream_read_data(&rstrm, srcaddr, ADR_WIDTH);
    stream_move_pointer(&rstrm, 1);                                            /* 协议类型 */
    stream_read_data(&rstrm, routeaddr, ADR_WIDTH);
    
    flowseq = stream_read_byte(&rstrm);                                        /* 读取入网流水号，终端每次请求入网自动加一 */

    node = net_link_node_find_exist(srcaddr);
    if (node == NULL) {
        isnew = true;
        node = net_link_node_new();
        if (node == NULL) {
            //_ack_0x01(packet, _FAILURE);                                       /* 拒绝入网应答 */
            return;
        }

        memcpy(node->addr, srcaddr, ADR_WIDTH);
    } else {
        isnew = false;
        if (node->flowseq == flowseq) {                                        /* 流水号一致，表示终端已经请求过，本次请求帧是其他路由转发的 */
            return;
        }
    }

    node->flowseq = flowseq;
    memcpy(node->routeaddr, routeaddr, ADR_WIDTH);

    node->type  = stream_read_byte(&rstrm);                                    /* 读取节点类型 */
    node->level = stream_read_byte(&rstrm);                                    /* 读取节点路由次数 */
    node->node_info.log_cnt++;

    if (isnew == true) {
        net_link_node_add(node);                                               /* 添加新加入的网络节点 */
    } else {
        net_link_node_update(srcaddr);
    }

    packet->node = node;
    _ack_0x01(packet, _SUCCESS);                                               /* 允许入网应答 */
}    

/*******************************************************************
** 函数名:     _hdl_0x02
** 函数描述:   链路维护请求 
** 参数:       [in]cmd    : 命令编码
**             [in]packet : 数据指针
** 返回:       无
********************************************************************/
static void _hdl_0x02(INT8U cmd, PACKET_BUF_T *packet)
{
    return_if_fail(packet->node != NULL);

#if SEND_MODE == PASSIVE_SEND
    if (net_link_send_inform(packet->node) == true) {                          /* 对于直连主节点的终端节点，由于终端节点只有发送完数据后的一段时间内处于接收状态(降低功耗)，因此下发的数据只能在收到心跳数据后发送 */
        return;
    }
#endif

    _ack_0x02(packet, _SUCCESS);
}

/*
********************************************************************************
* 注册回调函数
********************************************************************************
*/
static FUNCENTRY_LINK_T s_functionentry[] = {
        0x01,                   _hdl_0x01
       ,0x02,                   _hdl_0x02
};
static INT8U s_funnum = sizeof(s_functionentry) / sizeof(s_functionentry[0]);

static void _nrf24l01_cfg(void)
{
    Rf_para_t rf_para;

    public_para_manager_read_by_id(RF_PARA_, (INT8U*)&rf_para, sizeof(Rf_para_t));
    SYS_DEBUG("<ch:%d,dr:%x,pwr:%x>\n", rf_para.rf_ch, rf_para.rf_dr, rf_para.rf_pwr);
    
    if (nrf24l01_init(rf_para.rf_ch, rf_para.rf_dr, rf_para.rf_pwr)) {
        SYS_DEBUG("<---nrf24l01 init success>\n");
    } else {
        SYS_DEBUG("<---nrf24l01 init fail>\n");
    }

    nrf24l01_src_addr_set(0, s_local_addr, ADR_WIDTH);                         /* 设置本机地址 */
    nrf24l01_src_addr_set(1, (INT8U*)c_leve1_addr, ADR_WIDTH);                 /* 设置一级地址 */
    nrf24l01_mode_switch(RX);
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

    _nrf24l01_cfg();
}

/*******************************************************************
** 函数名:      net_link_init
** 函数描述:    链路层初始化
** 参数:        无
** 返回:        ture or false
********************************************************************/
BOOLEAN net_link_init(void)
{
	INT8U i;

    _nrf24l01_cfg();

    public_para_manager_reg_change_informer(RF_PARA_, _para_change_informer);
    
    net_link_recv_init();
    net_link_send_init();
    net_link_node_init();

    for (i = 0; i < s_funnum; i++) {
        net_link_register(s_functionentry[i].index, s_functionentry[i].entryproc);
    }

    return true;
}

/*******************************************************************
** 函数名:      net_link_get_local_addr
** 函数描述:    获取本机地址
** 参数:        无
** 返回:        本机地址指针
********************************************************************/
INT8U* net_link_get_local_addr(void)
{
    return s_local_addr;
}

