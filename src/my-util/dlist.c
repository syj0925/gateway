/********************************************************************************
**
** 文件名:     dlist.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   实现链表数据结构
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2007/04/17 | 陈从华 |  创建该文件
**| 2008/10/13 | 任赋   |  移植车台程序到该文件,接口使用原车台接口
*********************************************************************************/
#include "sys_typedef.h"
#include "dlist.h"
/*******************************************************************
** 函数名:     dlist_check
** 函数描述:   检测链表的有效性
** 参数:       [in]  Lp:链表
** 返回:       true:    链表有效
**             false:   链表无效
********************************************************************/
BOOLEAN dlist_check(LIST_T *Lp, void *bptr, void *eptr, BOOLEAN needchecklp)
{
    INT32U    count;
    LISTNODE *curnode;
	
    if (Lp == 0) return FALSE;
    if (needchecklp == TRUE) {
        if (((void *)Lp < bptr) || ((void *)Lp > eptr)) return FALSE;//art090209
    }
    
    count = 0;
    curnode = Lp->Head;
    while(curnode != 0) {
        if (((void *)curnode < bptr) || ((void *)curnode > eptr)) return FALSE;//art090209
        if (++count > Lp->Item) return FALSE;
        curnode = curnode->next;
    }
    if (count != Lp->Item) return FALSE;
	
    count = 0;
    curnode = Lp->Tail;
    while(curnode != 0) {
        if (((void *)curnode < bptr) || ((void *)curnode > eptr)) return FALSE;//art090209
        if (++count > Lp->Item) return FALSE;
        curnode = curnode->prv;
    }
    if (count != Lp->Item) {
        return FALSE;
    } else {
        return TRUE;
    }
}

/*******************************************************************
** 函数名:     dlist_init
** 函数描述:   初始化链表
** 参数:       [in]  Lp:链表
** 返回:       true:    成功
**             false:   失败
********************************************************************/
BOOLEAN dlist_init(LIST_T *Lp)
{
    if (Lp == 0) return FALSE;
	
    Lp->Head = 0;
    Lp->Tail = 0;
    Lp->Item = 0;
    return TRUE;
}

/*******************************************************************
** 函数名:     dlist_item
** 函数描述:   获取链表节点个数
** 参数:       [in]  Lp:        链表
** 返回:       链表节点个数
********************************************************************/
INT32U dlist_item(LIST_T *Lp)
{
    if (Lp == 0) {
        return 0;
    } else {
        return (Lp->Item);
    }
}

/*******************************************************************
** 函数名:      dlist_is_exist
** 函数描述:    判定链表是否存在该节点
** 参数:        [in]  Lp:        链表
**              [in]  Bp:        指定节点
** 返回:        true or false
********************************************************************/
BOOLEAN dlist_is_exist(LIST_T *Lp, LISTMEM *Bp)
{
    INT16U i;
    LISTNODE *nextnode, *curnode;

    if (Lp == 0 || Bp == 0) return FALSE;

    nextnode = Lp->Head;
    curnode  = (LISTNODE *)(Bp - sizeof(NODE));
    
    for (i = 0; i < Lp->Item; i++) {
        if (nextnode == curnode) {
            return TRUE;
        }
        nextnode = nextnode->next;
    }

    return FALSE;
}

/*******************************************************************
** 函数名:     dlist_get_head
** 函数描述:   获取链表头节点
** 参数:       [in]  Lp: 链表
** 返回:       链表头节点; 如链表无节点, 则返回0
********************************************************************/
LISTMEM *dlist_get_head(LIST_T *Lp)
{
    if (Lp == 0 || Lp->Item == 0) {
        return 0;
    } else {
        return ((LISTMEM *)Lp->Head + sizeof(NODE));
    }
}

/*******************************************************************
** 函数名:     dlist_get_tail
** 函数描述:   获取链表尾节点
** 参数:       [in]  Lp:        链表
** 返回:       链表尾节点; 如链表无节点, 则返回0
********************************************************************/
LISTMEM *dlist_get_tail(LIST_T *Lp)
{
    if (Lp == 0 || Lp->Item == 0) {
        return 0;
    } else {
        return ((LISTMEM *)Lp->Tail + sizeof(NODE));
    }
}

/*******************************************************************
** 函数名:     dlist_next_ele
** 函数描述:   获取指定节点的后一节点
** 参数:       [in]  Bp: 链表当前节点
** 返回:       Bp的下一个节点; 如返回0, 则表示节点不存在
********************************************************************/
LISTMEM *dlist_next_ele(LISTMEM *Bp)
{
    LISTNODE *curnode;
	
    if (Bp == 0) return 0;
    curnode = (LISTNODE *)(Bp - sizeof(NODE));
    if ((curnode = curnode->next) == 0) {
        return 0;
    } else {
        return ((LISTMEM *)curnode + sizeof(NODE));
    }
}

/*******************************************************************
** 函数名:     dlist_prv_ele
** 函数描述:   获取指定节点的前一节点
** 参数:       [in]  Bp: 指定节点
** 返回:       返回指定节点Bp的前一节点; 如返回0, 则表示不存在前一节点
********************************************************************/
LISTMEM *dlist_prv_ele(LISTMEM *Bp)
{
    LISTNODE *curnode;

    if (Bp == 0) return 0;
    curnode = (LISTNODE *)(Bp - sizeof(NODE));
    if ((curnode = curnode->prv) == 0) {
        return 0;
    } else {
        return ((LISTMEM *)curnode + sizeof(NODE));
    }
}

/*******************************************************************
** 函数名:     dlist_del_ele
** 函数描述:   删除指定节点
** 参数:       [in]  Lp:        链表
**             [in]  Bp:        指定节点
** 返回:       返回指定节点Bp的下个节点; 如返回0, 则表示Bp不存在下一节点
********************************************************************/
LISTMEM *dlist_del_ele(LIST_T *Lp, LISTMEM *Bp)
{
    LISTNODE *curnode, *prvnode, *nextnode;

    if (Lp == 0 || Bp == 0) return 0;
    if (Lp->Item == 0) return 0;

    Lp->Item--;
    curnode  = (LISTNODE *)(Bp - sizeof(NODE));
    prvnode  = curnode->prv;
    nextnode = curnode->next;
    if (prvnode == 0) {
        Lp->Head = nextnode;
    } else {
        prvnode->next = nextnode;
    }
    if (nextnode == 0) {
        Lp->Tail = prvnode;
        return 0;
    } else {
        nextnode->prv = prvnode;
        return ((LISTMEM *)nextnode + sizeof(NODE));
    }
}

/*******************************************************************
** 函数名:     dlist_del_head
** 函数描述:   删除链表头节点
** 参数:       [in]  Lp:        链表
** 返回:       链表头节点; 如返回0, 则表示不存在链表头节点
********************************************************************/
LISTMEM *dlist_del_head(LIST_T *Lp)
{
    LISTMEM *Bp;

    if (Lp == 0 || Lp->Item == 0) return 0;

    Bp = (LISTMEM *)Lp->Head + sizeof(NODE);
    dlist_del_ele(Lp, Bp);
    return Bp;
}

/*******************************************************************
** 函数名:     dlist_del_tail
** 函数描述:   删除链表尾节点
** 参数:       [in]  Lp:        链表
** 返回:       链表尾节点; 如返回0, 则表示不存在链表尾节点
********************************************************************/
LISTMEM *dlist_del_tail(LIST_T *Lp)
{
    LISTMEM *Bp;

    if (Lp == 0 || Lp->Item == 0) return 0;

    Bp = (LISTMEM *)Lp->Tail + sizeof(NODE);
    dlist_del_ele(Lp, Bp);
    return Bp;
}

/*******************************************************************
** 函数名:     dlist_append_ele
** 函数描述:   在链表尾上追加一个节点
** 参数:       [in]  Lp:        链表
**             [in]  Bp:        待追加节点
** 返回:       追加成功或失败
********************************************************************/
BOOLEAN dlist_append_ele(LIST_T *Lp, LISTMEM *Bp)
{
    LISTNODE *curnode;

    if (Lp == 0 || Bp == 0) return FALSE;

    curnode = (LISTNODE *)(Bp - sizeof(NODE));
    curnode->prv = Lp->Tail;
    if (Lp->Item == 0) {
        Lp->Head = curnode;
    } else {
        Lp->Tail->next = curnode;
    }
    curnode->next = 0;
    Lp->Tail = curnode;
    Lp->Item++;
    return TRUE;
}

/*******************************************************************
** 函数名:     dlist_insert_head
** 函数描述:   在链表头插入一个节点
** 参数:       [in]  Lp:        链表
**             [in]  Bp:        待插入的节点
** 返回:       插入成功或失败
********************************************************************/
BOOLEAN dlist_insert_head(LIST_T *Lp, LISTMEM *Bp)
{
    LISTNODE *curnode;

    if (Lp == 0 || Bp == 0) return FALSE;

    curnode = (LISTNODE *)(Bp - sizeof(NODE));
    curnode->next = Lp->Head;
    if (Lp->Item == 0) {
        Lp->Tail = curnode;
    } else {
        Lp->Head->prv = curnode;
    }
    curnode->prv = 0;
    Lp->Head = curnode;
    Lp->Item++;
    return TRUE;
}

/*******************************************************************
** 函数名:     dlist_connect_head_tail
** 函数描述:   将list首尾相连,形成环形
** 参数:       [in]  Lp:        链表
** 返回:       连接成功或失败
********************************************************************/
BOOLEAN dlist_connect_head_tail(LIST_T *Lp)
{
    if (Lp == 0) return FALSE;
    Lp->Head->prv  = Lp->Tail;
    Lp->Tail->next = Lp->Head;
    return true;
}

// 插入到CurBp的前面,即 InsBp->CurBp
/*******************************************************************
** 函数名:     dlist_insert_prv_ele
** 函数描述:   在指定节点前插入一个新节点
** 参数:       [in]  Lp:        链表
**             [in]  CurBp:     指定节点
**             [in]  InsBp:     待插入节点
** 返回:       插入成功或失败
********************************************************************/
BOOLEAN dlist_insert_prv_ele(LIST_T *Lp, LISTMEM *CurBp, LISTMEM *InsBp)
{
    LISTNODE *curnode, *insnode;
	
    if (Lp == 0 || CurBp == 0 || InsBp == 0) return FALSE;
    if (Lp->Item == 0) return FALSE;

    curnode  = (LISTNODE *)(CurBp - sizeof(NODE));
    insnode  = (LISTNODE *)(InsBp - sizeof(NODE));

    insnode->next = curnode;
    insnode->prv  = curnode->prv;
    if (curnode->prv == 0){
        Lp->Head = insnode;
    } else {
        curnode->prv->next = insnode;
    }
    curnode->prv = insnode;
    Lp->Item++;
    return TRUE;
}

/*******************************************************************
** 函数名:     dlist_insert_next_ele
** 函数描述:   在指定节点后插入一个新节点
** 参数:       [in]  Lp:        链表
**             [in]  CurBp:     指定节点
**             [in]  InsBp:     待插入节点
** 返回:       插入成功或失败
********************************************************************/
BOOLEAN dlist_insert_next_ele(LIST_T *Lp, LISTMEM *CurBp, LISTMEM *InsBp)
{
    LISTNODE *curnode, *insnode;

    if (Lp == 0 || CurBp == 0 || InsBp == 0) return FALSE;
    if (Lp->Item == 0) return FALSE;
    
    curnode = (LISTNODE *)(CurBp - sizeof(NODE));
    insnode = (LISTNODE *)(InsBp - sizeof(NODE));
    
    insnode->next = curnode->next;
    insnode->prv  = curnode;
    if(curnode->next == 0) {
        Lp->Tail = insnode;
    } else {
        curnode->next->prv = insnode;
    }
    curnode->next = insnode;
    Lp->Item++;
    return TRUE;
}

/*******************************************************************
** 函数名:     dlist_mem_init
** 函数描述:   将一块内存初始化成链表缓冲区
** 参数:       [in]  memLp:     链表
**             [in]  addr:      内存起始地址
**             [in]  nblks:     内存块个数
**             [in]  blksize:   内存块大小
** 返回:       成功或失败
********************************************************************/
BOOLEAN dlist_mem_init(LIST_T *memLp, LISTMEM *addr, INT32U nblks, INT32U blksize)
{
    if (!dlist_init(memLp)) return FALSE;

    addr += sizeof(NODE);
    for(; nblks > 0; nblks--){
        if (!dlist_append_ele(memLp, addr)) return FALSE;
        addr += blksize;
    }
    return TRUE;
}
