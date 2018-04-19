/********************************************************************************
**
** 文件名:     os_msg.h
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
#ifndef H_OS_MSG
#define H_OS_MSG

/*
********************************************************************************
* 数据结构定义
********************************************************************************
*/


/*
****************************************************************
*   定义各个函数申明
****************************************************************
*/
/* 消息处理函数声明 */
#ifdef BEGIN_MSG_DEF
#undef BEGIN_MSG_DEF
#endif

#ifdef MSG_DEF
#undef MSG_DEF
#endif

#ifdef END_MSG_DEF
#undef END_MSG_DEF
#endif

#define BEGIN_MSG_DEF(_TSK_ID_, MAX_MSG_QUEUE)

#define MSG_DEF(_TSK_ID_, _MSG_ID_, _PROC_)                    \
                   void _PROC_(INT32U lpara, INT32U hpara, void* p);
                   
#define END_MSG_DEF(_TSK_ID_)

#include "os_msg_reg.def"

/*
********************************************************************************
* 定义统一编号的任务ID
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


#define BEGIN_MSG_DEF(_TSK_ID_, MAX_MSG_QUEUE)    \
                   _TSK_ID_,
                    

#define MSG_DEF(_TSK_ID_, _MSG_ID_, _PROC_)
#define END_MSG_DEF(_TSK_ID_)

typedef enum {
    #include "os_msg_reg.def"
    TSK_ID_MAX
} OS_TSK_ID_E;

/*
********************************************************************************
* 定义统一编号的消息ID
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


#define BEGIN_MSG_DEF(_TSK_ID_, MAX_MSG_QUEUE) \
        typedef enum {

#define MSG_DEF(_TSK_ID_, _MSG_ID_, _PROC_) \
                    _MSG_ID_,
                    
#define END_MSG_DEF(_TSK_ID_)   \
          _TSK_ID_##_MSG_ID_MAX \
        } _TSK_ID_##_MSG_ID_E;

#include "os_msg_reg.def"


/*******************************************************************
** 函数名:     os_msg_post
** 函数描述:   发送系统消息
** 参数:       [in]  msgid:             系统消息ID
**             [in]  lpara:          低32位消息参数
**             [in]  hpara:          高32位消息参数
** 返回:       无
********************************************************************/
BOOLEAN os_msg_post(INT8U tskid, INT32U msgid, INT32U lpara, INT32U hpara, void*p);

/*******************************************************************
** 函数名:     os_msg_sched
** 函数描述:   处理系统消息
** 参数:       无
** 返回:       无
********************************************************************/
void os_msg_sched(void);

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
//void OS_APostMsg(BOOLEAN overlay, INT8U tskid, INT32U msgid, INT32U lpara, INT32U hpara);




#endif          /* end of H_GPS_MSGMAN */


