/********************************************************************************
**
** 文件名:     os_msg.c
** 版权所有:   
** 文件描述:   消息管理模块
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2012/08/14 | 苏友江 |  创建该文件
*********************************************************************************/
#include "sys_includes.h"

/*
********************************************************************************
* 数据结构定义
********************************************************************************
*/
/* 定义消息表项,用于存放注册的消息 */
typedef struct {
    INT16U  tskid;           /* 统一的任务编号 */
    INT16U  msgid;           /* 统一的消息编号 */
    void    (*proc)(INT32U lpara, INT32U hpara, void* p); /* 消息处理函数 */
} OS_MSG_TBL_T;

/* 定义任务表项，用于存放注册的任务 */
typedef struct {
    INT16U         tskid;    /* 统一的任务编号 */
    OS_MSG_TBL_T  *pmsg;     /* 各个任务的消息注册表 */
    INT32U         nmsg;     /* 各个任务的注册消息的个数 */
} OS_TSK_TBL_T;

typedef struct {
    INT8U          tskid;
    INT32U         msgid;
    INT32U         lpara;
    INT32U         hpara;
    void*          p;
} OS_MSG_T;

typedef struct {
    INT8U          flag;
    INT16U         msgpos;
    INT16U         nummsg;
    INT16U   const msgmax;
    OS_MSG_T*const pmsgq;
} MSGCB_T;

/*
****************************************************************
*   定义各个任务的消息的注册表
****************************************************************
*/
#ifdef BEGIN_MSG_DEF
#undef BEGIN_MSG_DEF
#endif

#ifdef MSG_DEF
#undef MSG_DEF
#endif

#ifdef END_MSG_DEF
#undef END_MSG_DEF
#endif

#define BEGIN_MSG_DEF(_TSK_ID_, MAX_MSG_QUEUE)          \
                   static const OS_MSG_TBL_T _TSK_ID_##MSG_TBL[] = {
                    
#define MSG_DEF(_TSK_ID_, _MSG_ID_, _PROC_)             \
                   {_TSK_ID_, _MSG_ID_, _PROC_},
                 
#define END_MSG_DEF(_TSK_ID_)                           \
                   {0}};

#include "os_msg_reg.def"

/*
****************************************************************
*   定义任务的注册表
****************************************************************
*/
#ifdef BEGIN_MSG_DEF
#undef BEGIN_MSG_DEF
#endif

#ifdef MSG_DEF
#undef MSG_DEF
#endif

#ifdef END_MSG_DEF
#undef END_MSG_DEF
#endif

#define BEGIN_MSG_DEF(_TSK_ID_, MAX_MSG_QUEUE)                              \
                   {_TSK_ID_,                                               \
                   (OS_MSG_TBL_T *)_TSK_ID_##MSG_TBL,                       \
                   sizeof(_TSK_ID_##MSG_TBL) / sizeof(OS_MSG_TBL_T) - 1     \
                   },
                   
#define MSG_DEF(_TSK_ID_, _MSG_ID_, _PROC_)
#define END_MSG_DEF(_TSK_ID_)

static const OS_TSK_TBL_T s_tsk_tbl[] = {
    #include "os_msg_reg.def"
    {0}
};

/*
****************************************************************
*   
****************************************************************
*/

#ifdef BEGIN_MSG_DEF
#undef BEGIN_MSG_DEF
#endif

#ifdef MSG_DEF
#undef MSG_DEF
#endif

#ifdef END_MSG_DEF
#undef END_MSG_DEF
#endif

#define BEGIN_MSG_DEF(_TSK_ID_, MAX_MSG_QUEUE)  \
                   static OS_MSG_T _TSK_ID_##MSG_QUEUE[MAX_MSG_QUEUE];
                    
#define MSG_DEF(_TSK_ID_, _MSG_ID_, _PROC_)
                 
#define END_MSG_DEF(_TSK_ID_)

#include "os_msg_reg.def"

/*
********************************************************************************
* 定义模块局部变量
********************************************************************************
*/
#ifdef BEGIN_MSG_DEF
#undef BEGIN_MSG_DEF
#endif

#ifdef MSG_DEF
#undef MSG_DEF
#endif

#ifdef END_MSG_DEF
#undef END_MSG_DEF
#endif

#define BEGIN_MSG_DEF(_TSK_ID_, MAX_MSG_QUEUE)  \
                   {0,                          \
                    0,                          \
                    0,                          \
                    MAX_MSG_QUEUE,              \
                    (OS_MSG_T*)_TSK_ID_##MSG_QUEUE \
                   },
#define MSG_DEF(_TSK_ID_, _MSG_ID_, _PROC_)
                 
#define END_MSG_DEF(_TSK_ID_)

static MSGCB_T s_msgcb_tab[] = {
    #include "os_msg_reg.def"
    {0}
};

/*******************************************************************
** 函数名:     _get_reg_msg_info
** 函数描述:   获取对应任务的注册信息
** 参数:       [in] tskid:统一编号的任务号
** 返回:       成功返回注册表指针，失败返回0
********************************************************************/
static OS_TSK_TBL_T* _get_reg_msg_info(INT16U tskid)
{
    if (tskid >= TSK_ID_MAX) {
        return 0;
    }

    return (OS_TSK_TBL_T *)(&s_tsk_tbl[tskid]);
}

/*******************************************************************
** 函数名:     os_msg_post
** 函数描述:   发送系统消息
** 参数:       [in]  msgid:             系统消息ID
**             [in]  lpara:          低32位消息参数
**             [in]  hpara:          高32位消息参数
** 返回:       无
********************************************************************/
BOOLEAN os_msg_post(INT8U tskid, INT32U msgid, INT32U lpara, INT32U hpara, void*p)
{
    INT16U pos;
    MSGCB_T *ptr;
    OS_MSG_T *pmsg;
    return_val_if_fail((tskid < TSK_ID_MAX), FALSE);
    
    ptr = &s_msgcb_tab[tskid];
    if (ptr->nummsg >= ptr->msgmax) {
        return false;
    }

    pos = ptr->msgpos + ptr->nummsg;
    if (pos >= ptr->msgmax) {
        pos -= ptr->msgmax;
    }
    pmsg = ptr->pmsgq;
    pmsg[pos].tskid = tskid;
    pmsg[pos].msgid = msgid;
    pmsg[pos].lpara = lpara;
    pmsg[pos].hpara = hpara;
    pmsg[pos].p     = p;
    if (ptr->flag == 0) {
        ptr->flag = 1;
        //PORT_PostCommonMsg();
    }
    ptr->nummsg++;
    return true;
}

/*******************************************************************
** 函数名:     MsgIsExist
** 函数描述:   测试在消息队列中是否存在指定的消息ID
** 参数:       [in]  msgid:             待测试的系统消息ID
** 返回:       true:  在消息队列中存在
**             false: 不在消息队列中存在
********************************************************************/
/*
static BOOLEAN MsgIsExist(INT8U tskid, INT32U msgid)
{
    INT16U pos, i;

    pos = s_mcb.msgpos;
    for (i = 0; i < s_mcb.nummsg; i++, pos++) {
        if (pos >= OS_MAX_MSG) {
            pos -= OS_MAX_MSG;
        }
        if ((s_mcb.msgq[pos].msgid == msgid) && (s_mcb.msgq[pos].tskid == tskid)) {
            return true;
        }
    }
    return false;
}
*/
/*******************************************************************
** 函数名:     OS_APostMsg
** 函数描述:   发送系统消息
** 参数:       [in]  overlay:           true:  如在消息队列中存在相同的消息ID, 则不将新的消息存放在消息队列中
**                                      false: 新的消息必须存放在消息队列中
**             [in]  tskid:            消息所属任务ID
**             [in]  msgid:             系统消息ID
**             [in]  lpara:          低32位消息参数
**             [in]  hpara:          高32位消息参数
** 返回:       无
********************************************************************/
/*
void OS_APostMsg(BOOLEAN overlay, INT8U tskid, INT32U msgid, INT32U lpara, INT32U hpara)
{
    if (overlay) {
        if (!MsgIsExist(tskid, msgid)) {
            os_msg_post(tskid, msgid, lpara, hpara);
        }
    } else {
        os_msg_post(tskid, msgid, lpara, hpara);
    }
}
*/
/*******************************************************************
** 函数名:     os_msg_sched
** 函数描述:   处理系统消息
** 参数:       无
** 返回:       无
********************************************************************/
void os_msg_sched(void)
{
    INT8U tskid;
    INT32U msgid, lpara, hpara, num;//, nmsg;
    OS_TSK_TBL_T *ptsk;
    OS_MSG_TBL_T *pmsg;
    MSGCB_T *ptr;
    OS_MSG_T *p;
    void* msg;

    for (tskid = 0; tskid < TSK_ID_MAX; tskid++) {
        ptr = &s_msgcb_tab[tskid];
        if (ptr->flag == 0) {
            continue;
        }
        
        p = ptr->pmsgq;
        for (;;) {
            num  = ptr->nummsg;
            if (num == 0) {
                ptr->flag = 0;
                return;
            }
            //tskid = p[ptr->msgpos].tskid;
            msgid = p[ptr->msgpos].msgid;
            lpara = p[ptr->msgpos].lpara;
            hpara = p[ptr->msgpos].hpara;
            msg   = p[ptr->msgpos].p;
            if (++ptr->msgpos >= ptr->msgmax) {
                ptr->msgpos = 0;
            }
            ptr->nummsg--;

            if (tskid < TSK_ID_MAX) {
                ptsk = _get_reg_msg_info(tskid);
                pmsg = ptsk->pmsg;
                //nmsg = ptsk->nmsg;

                if (pmsg[msgid].proc != 0) {
                    pmsg[msgid].proc(lpara, hpara, msg);
                }
            }
        }
    }
}

/*******************************************************************
** 函数名:     OS_MsgNULL
** 函数描述:   空消息处理函数，用来替换还未实现的消息处理函数
** 参数:       [in]  taskid:   任务id号
** 返回:       无
********************************************************************/
void OS_MsgNULL(INT32U lpara, INT32U hpara, void *p)
{
}

