/********************************************************************************
**
** 文件名:     drv_zigbee.c
** 版权所有:   (c) 2013-2016 
** 文件描述:   zigbee驱动
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**|    日期    |  作者  |  修改记录
**===============================================================================
**| 2016/06/14 | 苏友江 |  创建该文件
********************************************************************************/
#include "bsp.h"
#include "sys_includes.h"
#if EN_ZIGBEE > 0

#if DBG_ZIGBEE > 0
#define SYS_DEBUG          OS_DEBUG
#else
#define SYS_DEBUG(...)     do{}while(0)
#endif

/*
********************************************************************************
* 参数配置
********************************************************************************
*/
#define READ_WAIT   1*SECOND

typedef enum {
    INIT_START,
    INIT_CHA_CHK,
    INIT_CHA_CHKING,
    INIT_CHA_CFG,
    INIT_CHA_CFGING,
    INIT_NID_CHK,
    INIT_NID_CHKING,
    INIT_NID_CFG,
    INIT_NID_CFGING,
    INIT_RESTART,
    INIT_RESTARTING,
    INIT_FAIL,
    INIT_OK,
} Rtu_init_e;

/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
typedef struct {
    INT8U   link_tmr;
    
    INT8U   step;
    BOOLEAN restart;
    INT8U   initcnt;
    INT16U  waittime;

    INT8U   channel;
    INT16U  netid;

} Priv_t;

/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/
static Priv_t s_priv;
static INT8U s_recv_buf[100];
static RoundBuf_t s_roundbuf = {0};

extern Sys_base_info_t g_sys_base_info;

/* 最好在s_roundbuf初始化后，在开启usart2的接收中断，否则有可能存在隐患 */
void USART2_IRQHandler(void)
{
	if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) {
		roundbuf_write_byte(&s_roundbuf, (USART_ReceiveData(USART2) & 0xFF));
	}
}

static INT32S _usart2_write(INT8U* pdata, INT16U datalen)
{
    INT32S ret = datalen;

    while (datalen-- > 0) {
		while (USART_GetFlagStatus(EVAL_COM2, USART_FLAG_TC) == RESET);
		USART_SendData(EVAL_COM2, *pdata++);
    }

    return ret;
}

static INT32S _usart2_read(INT8U* pdata, INT16U datalen)
{
	INT32S ret = 0;
    INT16S recv;

    while (datalen-- > 0) {
        if ((recv = roundbuf_read_byte(&s_roundbuf)) == -1) {
            break;
        }
        pdata[ret++] = recv;
    }
    return ret;      
}

/*******************************************************************
** 函数名:     _link_tmr
** 函数描述:   link定时器
** 参数:       [in] index  : 定时器参数
** 返回:       无
********************************************************************/
static void _link_tmr(void *index)
{
    char strbuf[10];
    INT8U wbuf[50], rbuf[50];
    INT16S rlen;
    Stream_t wstrm;
    index = index;

    switch (s_priv.step) {
        case INIT_START:
        {
            SYS_DEBUG("<_link_tmr: INIT_START>\n");
            s_priv.restart = 0;
            s_priv.step = INIT_CHA_CHK;
            /* 判断初始化次数 */
            if (s_priv.initcnt++ > 50) {
                s_priv.initcnt = 0;
                s_priv.step = INIT_FAIL;
            }
            
            os_timer_start(s_priv.link_tmr, 1);
            break;
        }
        case INIT_CHA_CHK:
        {
            SYS_DEBUG("<_link_tmr: INIT_CHA_CHK>\n");
            
            stream_init(&wstrm, wbuf, sizeof(wbuf));
            stream_write_string(&wstrm, "AT+CHA?\n");
            _usart2_write(wbuf, stream_get_len(&wstrm));
            
            s_priv.step = INIT_CHA_CHKING;
            os_timer_start(s_priv.link_tmr, READ_WAIT);
            break;
        }
        case INIT_CHA_CHKING:
        {
            SYS_DEBUG("<_link_tmr: INIT_CHA_CHKING>\n");
            
            rlen = _usart2_read(rbuf, sizeof(rbuf));
            if (rlen > 0) {
                sprintf(strbuf, "%d", s_priv.channel);
                if (mem_find_str(rbuf, rlen, strbuf)) {
                    s_priv.step = INIT_NID_CHK;
                } else {
                    s_priv.step = INIT_CHA_CFG;
                }
            } else {
                s_priv.step = INIT_START;
            }
            os_timer_start(s_priv.link_tmr, 1);
            break;
        }
        case INIT_CHA_CFG:
        {
            SYS_DEBUG("<_link_tmr: INIT_CHA_CFG>\n");

            stream_init(&wstrm, wbuf, sizeof(wbuf));
            stream_write_sprintf(&wstrm, "AT+CHA=%d\n", s_priv.channel);
            _usart2_write(wbuf, stream_get_len(&wstrm));

            s_priv.step = INIT_CHA_CFGING;
            os_timer_start(s_priv.link_tmr, READ_WAIT);
            break;
        }
        case INIT_CHA_CFGING:
        {
            SYS_DEBUG("<_link_tmr: INIT_CHA_CFGING>\n");
            os_timer_start(s_priv.link_tmr, 1);
            rlen = _usart2_read(rbuf, sizeof(rbuf));
            if (rlen > 0) {
                sprintf(strbuf, "%d", s_priv.channel);
                if (mem_find_str(rbuf, rlen, strbuf)) {
                    s_priv.restart = 1;                                        /* 参数已经修改，需要复位操作 */
                    s_priv.step = INIT_NID_CHK;
                    break;
                }
            }
            s_priv.step = INIT_START;
            break;
        }        
        case INIT_NID_CHK:
        {
            SYS_DEBUG("<_link_tmr: INIT_NID_CHK>\n");
            
            /* 查询网络号，并与本机对比 */
            stream_init(&wstrm, wbuf, sizeof(wbuf));
            stream_write_string(&wstrm, "AT+NID?\n");
            _usart2_write(wbuf, stream_get_len(&wstrm));
            
            s_priv.step = INIT_NID_CHKING;
            os_timer_start(s_priv.link_tmr, READ_WAIT);
            break;
        }
        case INIT_NID_CHKING:
        {
            SYS_DEBUG("<_link_tmr: INIT_NID_CHKING>\n");
            
            rlen = _usart2_read(rbuf, sizeof(rbuf));
            if (rlen > 0) {
                sprintf(strbuf, "%d", s_priv.netid);
                if (mem_find_str(rbuf, rlen, strbuf)) {
                    s_priv.step = INIT_RESTART;
                } else {
                    s_priv.step = INIT_NID_CFG;
                }
            } else {
                s_priv.step = INIT_START;
            }
            os_timer_start(s_priv.link_tmr, 1);
            break;
        }
        case INIT_NID_CFG:
        {
            SYS_DEBUG("<_link_tmr: INIT_NID_CFG>\n");

            stream_init(&wstrm, wbuf, sizeof(wbuf));
            stream_write_sprintf(&wstrm, "AT+NID=%d\n", s_priv.netid);
            _usart2_write(wbuf, stream_get_len(&wstrm));

            s_priv.step = INIT_NID_CFGING;
            os_timer_start(s_priv.link_tmr, READ_WAIT);
            break;
        }
        case INIT_NID_CFGING:
        {
            SYS_DEBUG("<_link_tmr: INIT_NID_CFGING>\n");
            os_timer_start(s_priv.link_tmr, 1);
            rlen = _usart2_read(rbuf, sizeof(rbuf));
            if (rlen > 0) {
                sprintf(strbuf, "%d", s_priv.netid);
                if (mem_find_str(rbuf, rlen, strbuf)) {
                    s_priv.restart = 1;                                        /* 参数已经修改，需要复位操作 */
                    s_priv.step = INIT_RESTART;
                    break;
                }
            }
            s_priv.step = INIT_RESTART;
            break;
        }  
        case INIT_RESTART:
        {
            SYS_DEBUG("<_link_tmr: INIT_RESTART>\n");

            if (s_priv.restart) {
                stream_init(&wstrm, wbuf, sizeof(wbuf));
                stream_write_string(&wstrm, "AT+SRS\n");
                _usart2_write(wbuf, stream_get_len(&wstrm));

                s_priv.step = INIT_RESTARTING;
                os_timer_start(s_priv.link_tmr, READ_WAIT);
            } else {
                s_priv.step = INIT_OK;
                os_timer_start(s_priv.link_tmr, 1);
            }
            break;
        }
        case INIT_RESTARTING:
        {
            SYS_DEBUG("<_link_tmr: INIT_RESTARTING>\n");
            os_timer_start(s_priv.link_tmr, 1);
            rlen = _usart2_read(rbuf, sizeof(rbuf));
            if (rlen > 0) {
                if (mem_find_str(rbuf, rlen, "srs")) {
                    s_priv.restart = 0;
                    s_priv.step = INIT_OK;
                    break;
                }
            }
            s_priv.step = INIT_START;
            break;
        } 
        case INIT_FAIL:
        {
            SYS_DEBUG("<_link_tmr: INIT_FAIL>\n");
            os_timer_start(s_priv.link_tmr, 10*SECOND);
            if (s_priv.waittime++ > 6) {
                s_priv.waittime = 0;
                s_priv.step = INIT_START;
            }
            break;
        }
        case INIT_OK:
        {
            SYS_DEBUG("<_link_tmr: INIT_OK>\n");
            os_timer_stop(s_priv.link_tmr);
            break;
        }
        default:
            break;
    }
}

/*******************************************************************
** 函数名:      drv_zigbee_init
** 函数描述:    
** 参数:        none
** 返回:        none
********************************************************************/
void drv_zigbee_init(void)
{
    memset(&s_priv, 0, sizeof(Priv_t));
    s_priv.channel = 21;
    s_priv.netid   = 30;
    
    s_priv.link_tmr = os_timer_create(0, _link_tmr);
    os_timer_start(s_priv.link_tmr, 1*SECOND);

    roundbuf_init(&s_roundbuf, s_recv_buf, sizeof(s_recv_buf), NULL); 
    /* bug:(可能存在bug)最好在循环缓冲区初始化后，在开启usart2中断 */
    usart_rxit_enable(COM2);
}

INT32S drv_zigbee_write(INT8U* pdata, INT16U datalen)
{
    if (s_priv.step != INIT_OK) {
        return -1;
    }
    
    return _usart2_write(pdata, datalen);
}

INT32S drv_zigbee_read(INT8U* pdata, INT16U datalen)
{
    if (s_priv.step != INIT_OK) {
        return -1;
    }

    return _usart2_read(pdata, datalen);
}

BOOLEAN drv_zigbee_is_connect(void)
{
    if (s_priv.step == INIT_OK) {
        return true;
    } else {
        return false;
    }
}

#endif /* end of EN_ZIGBEE */

