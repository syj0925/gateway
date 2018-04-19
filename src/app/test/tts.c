/********************************************************************************
**
** 文件名:     tts.c
** 版权所有:   (c) 2013-2017 
** 文件描述:   TTS语音播报
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**|    日期    |  作者  |  修改记录
**===============================================================================
**| 2017/08/14 | 苏友江 |  创建该文件
********************************************************************************/
#include "bsp.h"
#include "sys_includes.h"

#if DBG_SYSTEM > 0
#define SYS_DEBUG          OS_DEBUG
#else
#define SYS_DEBUG(...)     do{}while(0)
#endif

static INT8U s_tts_tmr;
static INT8U s_num;

#define NUM_MAX     8

const INT8U cmd[NUM_MAX][6] = {
    {0xAA, 0x07, 0x02, 0x00, 0x01, 0xB4},
    {0xAA, 0x07, 0x02, 0x00, 0x02, 0xB5},
    {0xAA, 0x07, 0x02, 0x00, 0x03, 0xB6},
    {0xAA, 0x07, 0x02, 0x00, 0x04, 0xB7},
    {0xAA, 0x07, 0x02, 0x00, 0x05, 0xB8},
    {0xAA, 0x07, 0x02, 0x00, 0x06, 0xB9},
    {0xAA, 0x07, 0x02, 0x00, 0x07, 0xBA},
    {0xAA, 0x07, 0x02, 0x00, 0x08, 0xBB}
};

/*******************************************************************
** 函数名:     _tts_tmr
** 函数描述:   
** 参数:       [in] index  : 定时器参数
** 返回:       无
********************************************************************/
static void _tts_tmr(void *index)
{
    INT8U i;
    index = index;

    for (i = 0; i < 6; i++) {
		while (USART_GetFlagStatus(EVAL_COM3, USART_FLAG_TC) == RESET);
		USART_SendData(EVAL_COM3, cmd[s_num-1][i]);
    }
}

void tts_init(void)
{
    s_tts_tmr = os_timer_create(0, _tts_tmr);
}

BOOLEAN tts_play_stop(INT8U num)
{
    os_timer_stop(s_tts_tmr);
    return true;
}

BOOLEAN tts_play(INT8U num, INT8U mode)
{
    INT8U i;

    if (num > NUM_MAX || num == 0) {
        return false;
    }

    for (i = 0; i < 6; i++) {
		while (USART_GetFlagStatus(EVAL_COM3, USART_FLAG_TC) == RESET);
		USART_SendData(EVAL_COM3, cmd[num-1][i]);
    }

    if (mode == 1) {
        s_num = num;
        os_timer_start(s_tts_tmr, 30*SECOND);
    }

    return true;
}
    
