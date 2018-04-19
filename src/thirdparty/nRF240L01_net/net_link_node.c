/********************************************************************************
**
** 文件名:     net_link_node.c
** 版权所有:   (c) 2013-2014 
** 文件描述:   网络节点管理
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**|    日期    |  作者  |  修改记录
**===============================================================================
**| 2014/9/16 | 苏友江 |  创建该文件
********************************************************************************/
#include "sys_includes.h"
#include "net_typedef.h"
#include "net_link_node.h"

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
#define REG_MAX     4



/*
********************************************************************************
* 结构定义
********************************************************************************
*/
/* 网络节点链表 */
typedef struct {
    NODE            node;
    Net_link_node_t cell;
} NET_NODE_T;

typedef struct {
    LIST_T freelist;
    LIST_T onlinelist;
    LIST_T offlinelist;
    NET_NODE_T net_node[NET_NODE_MAX];
    Inform inform[REG_MAX];
    INT8U mantmr;
} LCB_T;

/*
********************************************************************************
* 静态变量
********************************************************************************
*/
static LCB_T s_lcb;

/*******************************************************************
** 函数名:     manager_tmr
** 函数描述:   节点管理定时器
** 参数:       [in] index  : 定时器参数
** 返回:       无
********************************************************************/
static void manager_tmr(void *index)
{
    INT8U i;
    Net_link_node_t *cell, *destcell;
    
    index = index;
    cell = (Net_link_node_t *)dlist_get_head(&s_lcb.onlinelist);
    for (;;) {
        if (cell == 0) {
            break;
        }
        
        if (cell->live == 0) {
            destcell = cell;
            cell = (Net_link_node_t *)dlist_next_ele((LISTMEM *)cell);
            
            destcell->status = OFFLINE;
            dlist_del_ele(&s_lcb.onlinelist, (LISTMEM *)destcell);
            dlist_append_ele(&s_lcb.offlinelist, (LISTMEM *)destcell);
            for (i = 0; i < REG_MAX; i++) {
                if (s_lcb.inform[i] == NULL) {
                    break;
                }
                s_lcb.inform[i](NODE_OFFLINE, destcell);
            }  
            continue;
        }
        cell->live--;    
        cell = (Net_link_node_t *)dlist_next_ele((LISTMEM *)cell);
    }
}

/*******************************************************************
** 函数名:      net_link_node_find_by_addr
** 函数描述:    根据地址查找网络节点
** 参数:        [in] plist: 指定链表
**              [in] addr : 查询地址       
** 返回:        Net_link_node_t
********************************************************************/
Net_link_node_t* net_link_node_find_by_addr(LIST_T *plist, INT8U *addr)
{
    Net_link_node_t *cell;

    cell = (Net_link_node_t *)dlist_get_head(plist);
    for (;;) {
        if (cell == 0) {
            break;
        }
        if (strncmp((char*)cell->addr, (char*)addr, ADR_WIDTH) == 0) {
            return cell;
        } 
        cell = (Net_link_node_t *)dlist_next_ele((LISTMEM *)cell);
    }

    return NULL;
}

/*******************************************************************
** 函数名:      net_link_node_find_by_addr
** 函数描述:    根据地址查找已经存在网络节点
** 参数:        [in] addr : 查询地址
** 返回:        Net_link_node_t
********************************************************************/
Net_link_node_t* net_link_node_find_exist(INT8U *addr)
{
    Net_link_node_t *cell;

    cell = net_link_node_find_by_addr(&s_lcb.onlinelist, addr);
    if (cell != NULL) {                                                        /* 在线链表中存在要更新的节点 */
        return cell;
    }

    cell = net_link_node_find_by_addr(&s_lcb.offlinelist, addr);
    if (cell != NULL) {                                                        /* 掉线链表中存在要更新的节点 */
        return cell;
    }

    return NULL;
}

/*******************************************************************
** 函数名:      net_link_node_update
** 函数描述:    只更新原有的网络节点，不创建
** 参数:        [in] addr     : 目标地址
** 返回:        Net_link_node_t
********************************************************************/
Net_link_node_t* net_link_node_update(INT8U *addr)
{
    INT8U i;
    Net_link_node_t *cell;

    cell = net_link_node_find_by_addr(&s_lcb.onlinelist, addr);
    if (cell != NULL) {                                                        /* 在线链表中存在要更新的节点 */
        cell->live = LIVE_TIME_MAX;
        return cell;
    }

    cell = net_link_node_find_by_addr(&s_lcb.offlinelist, addr);
    if (cell != NULL) {                                                        /* 掉线链表中存在要更新的节点 */
        dlist_del_ele(&s_lcb.offlinelist, (LISTMEM *)cell);
        dlist_append_ele(&s_lcb.onlinelist, (LISTMEM *)cell);
        
        cell->status = ONLINE;
        cell->live = LIVE_TIME_MAX;
        for (i = 0; i < REG_MAX; i++) {
            if (s_lcb.inform[i] == NULL) {
                break;
            }
            s_lcb.inform[i](NODE_ONLINE, cell);
        }    

        return cell;   
    }
    return NULL;
}

/*******************************************************************
** 函数名:      net_link_node_new
** 函数描述:    创建新网络节点
** 参数:        无      
** 返回:        Net_link_node_t
********************************************************************/
Net_link_node_t* net_link_node_new(void)
{
    INT8U i;
    Net_link_node_t *cell;

    cell = (Net_link_node_t *)dlist_del_head(&s_lcb.freelist);                 /* 链表中找不到要更新的节点，从空闲链表创建一个新网络节点 */
    if (cell != NULL) {
        memset(cell, 0, sizeof(Net_link_node_t));
        return cell;
    }

    cell = (Net_link_node_t *)dlist_del_head(&s_lcb.offlinelist);
    if (cell != NULL) {
        for (i = 0; i < REG_MAX; i++) {                                        /* 删除旧节点通知 */
            if (s_lcb.inform[i] == NULL) {
                continue;
            }
            s_lcb.inform[i](NODE_DEL, cell);
        } 
        
        memset(cell, 0, sizeof(Net_link_node_t));
        return cell;
    }
    return NULL;   
}

/*******************************************************************
** 函数名:      net_link_node_add
** 函数描述:    添加网络节点
** 参数:        [in] node: 要添加的节点
** 返回:        true or false
********************************************************************/
BOOLEAN net_link_node_add(Net_link_node_t* node)
{
    INT8U i;

    return_val_if_fail(node != NULL, FALSE);

    node->status = ONLINE;
    node->live   = LIVE_TIME_MAX;
    dlist_append_ele(&s_lcb.onlinelist, (LISTMEM *)node);
    
    for (i = 0; i < REG_MAX; i++) {
        if (s_lcb.inform[i] == NULL) {
            continue;
        }
        s_lcb.inform[i](NODE_ADD, node);
    }    
    return TRUE;  
}

/*******************************************************************
** 函数名:      net_link_node_init
** 函数描述:    网络节点管理初始化
** 参数:        无
** 返回:        无
********************************************************************/
void net_link_node_init(void)
{
    memset(&s_lcb, 0, sizeof(s_lcb));

    dlist_init(&s_lcb.onlinelist);
    dlist_init(&s_lcb.offlinelist);
    dlist_mem_init(&s_lcb.freelist, (LISTMEM *)s_lcb.net_node, NET_NODE_MAX, sizeof(NET_NODE_T));

    s_lcb.mantmr = os_timer_create(0, manager_tmr);
    os_timer_start(s_lcb.mantmr, 2*SECOND);
}

/*******************************************************************
** 函数名:      net_link_node_get_list
** 函数描述:    获取在线链表
** 参数:        [in] type: 链表类型
** 返回:        LIST_T
********************************************************************/
LIST_T* net_link_node_get_list(INT8U type)
{
    if (type == NODE_ONLINE) {
        return &s_lcb.onlinelist;
    } else {
        return &s_lcb.offlinelist;
    }
}

/*******************************************************************
** 函数名:      net_link_node_reg
** 函数描述:    注册节点添加/删除通知回调函数
** 参数:        [in] inform: 回调函数
** 返回:        true or false
********************************************************************/
BOOLEAN  net_link_node_reg(Inform inform)
{
    INT8U i;
    
    for (i = 0; i < REG_MAX; i++) {
        if (s_lcb.inform[i] == NULL) {
            break;
        }
    }

    return_val_if_fail((inform != NULL) && (i < REG_MAX), FALSE);

    s_lcb.inform[i] = inform;
	return TRUE;
}

