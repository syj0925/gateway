/********************************************************************************
**
** 文件名:     comm_protocol.h
** 版权所有:   (c) 2013-2015 
** 文件描述:   通信协议相关定义和申明
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**|    日期    |  作者  |  修改记录
**===============================================================================
**| 2015/09/09 | 苏友江 |  创建该文件
********************************************************************************/
#ifndef COMM_PROTOCOL_H
#define COMM_PROTOCOL_H

#ifdef GLOBALS_COMM_RECV
Asmrule_t const g_comm_rules = {0x7e, 0x7d, 0x02, 0x01};
#else
extern Asmrule_t const g_comm_rules;
#endif

typedef struct {
    INT8U   msgid[2];
    INT8U   msgatr[2];
    INT8U   uid[2];
    INT8U   flowseq[2];
    INT8U   data[1];
} Comm_frame_t;

/*
********************************************************************************
*                  DEFINE COMM FRAME HEAD LEN
********************************************************************************
*/
#define FRAMEHEAD_LEN                     (sizeof(Comm_frame_t) - 1)
#define FRAMETAIL_LEN                     1




BOOLEAN comm_protocol_common_ack(INT16U flowseq, INT16U msgid, INT8U result);

INT8U* comm_protocol_asm_status(Stream_t *wstrm);

#endif
