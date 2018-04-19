/********************************************************************************
**
** 文件名:     comm_protocol.c
** 版权所有:   (c) 2013-2015 
** 文件描述:   通信协议管理
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**|    日期    |  作者  |  修改记录
**===============================================================================
**| 2015/10/2 | 苏友江 |  创建该文件
********************************************************************************/
#include "bsp.h"
#include "sys_includes.h"



BOOLEAN comm_protocol_common_ack(INT16U flowseq, INT16U msgid, INT8U result)
{
    Stream_t wstrm;
    Comm_pkt_send_t send_pkt;
    INT8U sendbuf[2+2+1];

    stream_init(&wstrm, sendbuf, sizeof(sendbuf));
    stream_write_half_word(&wstrm, flowseq);
    stream_write_half_word(&wstrm, msgid);
    stream_write_byte(&wstrm, result);

    send_pkt.len   = sizeof(sendbuf);
    send_pkt.pdata = sendbuf;
    send_pkt.msgid = 0x0001;
    return comm_send_dirsend(&send_pkt);
}

INT8U* comm_protocol_asm_status(Stream_t *wstrm)
{
    #define BUFLEN    4+4+1+LAMP_NUM_MAX*7
    INT8U *ptr, i;
    
    ptr = (INT8U*)mem_malloc(BUFLEN);
    if (ptr == NULL) {
        return NULL;
    }    

    stream_init(wstrm, ptr, BUFLEN);
    stream_write_long(wstrm, alarm_get_status());                              /* 报警 */
    stream_write_long(wstrm, 0x00000000);                                      /* 状态 */
    stream_write_byte(wstrm, LAMP_NUM_MAX);

    for (i = 0; i < LAMP_NUM_MAX; i++) {
        stream_write_byte(wstrm, i);
        if (lamp_is_open((Lamp_e)i)) {
            stream_write_byte(wstrm, LAMP_OPEN);
            stream_write_byte(wstrm, lamp_get_dimming((Lamp_e)i));
        } else {
            stream_write_byte(wstrm, LAMP_CLOSE);
        }
        stream_write_long(wstrm, g_power.lamp[i].t_light);
    }

    return ptr;
}

