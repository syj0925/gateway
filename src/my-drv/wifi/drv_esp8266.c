/********************************************************************************
**
** 文件名:     drv_esp8266.c
** 版权所有:   (c) 2013-2016 
** 文件描述:   esp8266驱动，采用透传模式，方便开发，但是只能支持一条连接
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**|    日期    |  作者  |  修改记录
**===============================================================================
**| 2016/12/09 | 苏友江 |  创建该文件
********************************************************************************/
#include "bsp.h"
#include "sys_includes.h"

#if EN_WIFI > 0

#if DBG_WIFI > 0
#define SYS_DEBUG           OS_DEBUG
#else
#define SYS_DEBUG(...)      do{}while(0)
#endif

/*
********************************************************************************
* 注释 
********************************************************************************
*/


/*
********************************************************************************
* 定义模块配置参数
********************************************************************************
*/
#ifdef OK
#undef OK
#endif

#ifdef ERROR
#undef ERROR
#endif

#define OK 		"OK"
#define ERROR 	"ERROR"

static const char *modetbl[2] = {"TCP", "UDP"};                                /* 连接模式 */

/*
********************************************************************************
* 定义模块结构
********************************************************************************
*/

typedef enum {	
    /* 基础AT命令 */
	ATE,           /* 关闭回显功能 */
	ATRST,         /* 重启模块 */
	ATRESTORE,     /* 恢复出厂设置，模块会重启 */
	ATGMR,         /* 查询版本信息 */
	
    /* WIFI功能AT命令 */
	ATCWMODE,      /* 设置WIFI应用模式:1 Station 2 AP 3 AP+Station */
	ATCWDHCP,      /* 设置 DHCP开关 mode 0:设置  AP 1:设置  STA 2:设置  AP 和  STA en 0:去能  DHCP 1:使能  DHCP */
 
        /* STA相关 */
	ATCWLAP,       /* 列出当前可用的AP */
	ATCWJAP,       /* 加入AP <ssid 接入点名称 <pwd 密码最长64字节ASCII参数 */
	ATCWJAP_Q,     /* 查询是否加入AP */
	ATCWQAP,       /* 退出与AP的连接 */   
	ATCWAUTOCON,   /* 设置 STA开机自动连接 enable 0:开机禁能STA自动连接 1:开机使能STA自动连接 */     
	ATCIPSTAMAC,   /* 设置模块STA模式的 MAC地址 */     
	ATCIPSTA,      /* 设置模块STA模式的 IP地址  */     
    ATSMART_ON,    /* 启动智能连接 */
    ATSMART_OFF,   /* 停止智能连接 */

    /* TCPIP相关命令 */
	ATCIPSTATUS,   /* 获得连接状态 */
	ATCIPSTART,    /* 建立TCP连接或者注册UDP端口号 */
	ATCIPSEND,     /* 发送数据 */
	ATCIPCLOSE,    /* 关闭 TCP或 UDP */
	ATCIFSR,       /* 获得本地IP地址 */
	ATCIPMUX,      /* 启动多连接 */
	ATCIPSERVER,   /* 配置为服务器 */
	ATCIPMODE,     /* 设置模块传输模式 */
	ATCIPSTO,      /* 设置服务器超时时间 */
	ATPING,        /* PING命令 */ 
    
} Esp8266_e;

typedef enum {
    AT_STEP_FREE,                    /* 空闲状态 */
    AT_STEP_SEND,                    /* 发送at命令 */
    AT_STEP_RECV,                    /* 接收at应答命令 */
} At_step_e;  

typedef enum {
    STEP_POWER_OFF = 0,
    STEP_POWER_ON,
    STEP_QUIT_TRANS,                 /* 退出透传模式（esp8266断电上电后，默认是at模式，加上这个是为了保险） */
    STEP_CLOSE_TRANS,                /* 关闭透传模式 */
    STEP_SMART_ON,                   /* 开启smart config */
    STEP_SMART_WAIT,                 /* 等待smart config */
    STEP_SMART_OFF,                  /* 关闭smart config */
    STEP_SET_STA,                    /* 设置为STA模式 */
    STEP_RST,                        /* 重启 */
    STEP_CLOSE_ATE,                  /* 关闭回显示 */
    STEP_JOIN_AP,                    /* 加入AP */
    STEP_QUERY_AP,                   /* 查询AP状态 */
    STEP_TCP_CONNECT,                /* TCP连接 */
    STEP_OPEN_TRANS,                 /* 开启透传模式 */
    STEP_TRANS,                      /* 开始透传 */
    STEP_OK,
} Esp82666_step_e;

typedef struct {
    INT8U     at_tmr;
    At_step_e at_step;               /* 记录AT命令执行状态 */
    const At_cmd_t *at_cmd;          /* 当前要执行的AT命令 */
    char      para[AT_PARA_NUM][AT_PARA_SIZE]; /* AT命令参数 */
    INT16S    wait;                  /* 等待时间，单位ms */
    BOOLEAN   rflag;                 /* 接收数据标识，接收过数据，则置1 */
    INT16S    rlen;                  /* 接收数据长度 */
    INT8U    *precv;                 /* 指向当前接收位置 */
    INT8U     rbuf[128];             /* 接收数据缓存 */
    At_ret    ret;                   /* AT命令执行结果 */
    void (*fp)(At_ret result);       /* 执行结束回调 */
} At_t;

typedef struct {
    INT8U   link_tmr;
    INT8U   step;
    INT8U   failcnt;
    INT8U   qjapcnt;                /* 查询加网次数 */
    BOOLEAN smartcfg;
    
    Wifi_para_t para;
} Priv_t;

/*
********************************************************************************
* 定义模块配置参数
********************************************************************************
*/
static const At_cmd_t c_at_cmd_sets[]={
    /* 基础AT命令 */
	{ATE,        "ATE0\r\n",                      OK,      ERROR,    3000,    0,    NULL},           /* 关闭回显功能 */
	{ATRST,      "AT+RST\r\n",                    OK,      ERROR,    3000,    0,    NULL},           /* 重启模块 */
	{ATRESTORE,  "AT+RESTORE\r\n",                OK,      ERROR,    3000,    0,    NULL},           /* 恢复出厂设置，模块会重启 */
	{ATGMR,      "AT+GMR\r\n",                    OK,      ERROR,    3000,    0,    NULL},           /* 查询版本信息 */
	
    /* WIFI功能AT命令 */
	{ATCWMODE,   "AT+CWMODE=1\r\n",               OK,      ERROR,    3000,    0,    NULL},            /* 设置WIFI应用模式:1 Station 2 AP 3 AP+Station */
	{ATCWDHCP,   "AT+CWDHCP=1,1\r\n",             OK,      ERROR,    3000,    0,    NULL},            /* 设置DHCP开关 mode 0:设置AP 1:设置STA 2:设置AP和STA en 0:去能DHCP 1:使能DHCP */

        /* STA相关 */
	{ATCWLAP,    "AT+CWLAP\r\n",                  OK,      ERROR,    3000,    0,    NULL},            /* 列出当前可用的AP */
	{ATCWJAP,    "AT+CWJAP=\"$\",\"$\"\r\n",      OK,      "FAIL",  20000,    2,    NULL},            /* 加入AP <ssid 接入点名称 <pwd 密码最长64字节ASCII参数 */
	{ATCWJAP_Q,  "AT+CWJAP?\r\n",                 OK,      ERROR,    3000,    0,    NULL},            /* 查询是否加入AP */
	{ATCWQAP,    "AT+CWQAP\r\n",                  OK,      ERROR,    3000,    0,    NULL},            /* 退出与AP的连接 */   
	{ATCWAUTOCON,"AT+CWAUTOCONN=$\r\n",           OK,      ERROR,    3000,    1,    NULL},            /* 设置 STA开机自动连接 enable 0:开机禁能STA自动连接 1:开机使能STA自动连接 */     
	{ATCIPSTAMAC,"AT+CIPSTAMAC=\"$\"\r\n",        OK,      ERROR,    3000,    1,    NULL},            /* 设置模块STA模式的 MAC地址 */     
	{ATCIPSTA,   "AT+CIPSTA=\"$\"\r\n",           OK,      ERROR,    3000,    1,    NULL},            /* 设置模块STA模式的 IP地址  */     
	{ATSMART_ON, "AT+CWSMARTSTART=2\r\n",         OK,      ERROR,    3000,    0,    NULL},            /* 启动智能连接 */     
	{ATSMART_OFF,"AT+CWSMARTSTOP\r\n",            OK,      ERROR,    3000,    0,    NULL},            /* 停止智能连接 */     

    /* TCPIP相关命令 */
	{ATCIPSTATUS,"AT+CIPSTATUS\r\n",              OK,      ERROR,    3000,    0,    NULL},   /* 获得连接状态 */
	{ATCIPSTART, "AT+CIPSTART=\"$\",\"$\",$\r\n","CONNECT",ERROR,   10000,    3,    NULL},   /* 建立TCP连接或者注册UDP端口号 */
	{ATCIPSEND,  "AT+CIPSEND\r\n",               ">",      ERROR,    3000,    0,    NULL},   /* 发送数据 */
	{ATCIPCLOSE, "AT+CIPCLOSE=$\r\n",             OK,      ERROR,    3000,    1,    NULL},   /* 关闭TCP或UDP */
	{ATCIFSR,    "AT+CIFSR\r\n",                  OK,      ERROR,    3000,    0,    NULL},   /* 获得本地IP地址 */
	{ATCIPMUX,   "AT+CIPMUX=1\r\n",               OK,      ERROR,    3000,    0,    NULL},   /* 启动多连接 */
	{ATCIPSERVER,"AT+CIPSERVER=$,$\r\n",          OK,      ERROR,    3000,    2,    NULL},   /* 配置为服务器 */
	{ATCIPMODE,  "AT+CIPMODE=$\r\n",              OK,      ERROR,    3000,    1,    NULL},   /* 设置模块传输模式 */
	{ATCIPSTO,   "AT+CIPSTO=$\r\n",               OK,      ERROR,    3000,    1,    NULL},   /* 设置服务器超时时间 */
	{ATPING,     "AT+PING=\"$\"\r\n",             OK,      ERROR,    3000,    1,    NULL},   /* PING命令 */ 
     
};

static INT8U s_recv_buf[512];
static RoundBuf_t s_roundbuf = {0};
static At_t s_at;
static Priv_t s_priv;

/* 最好在s_roundbuf初始化后，在开启usart2的接收中断，否则有可能存在隐患 */
void USART2_IRQHandler(void)
{
	if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) {
		roundbuf_write_byte(&s_roundbuf, (USART_ReceiveData(USART2) & 0xFF));
	}
}

static INT32S _conn_write(INT8U* pdata, INT16U datalen)
{
    INT32S ret = datalen;

    while (datalen-- > 0) {
		while (USART_GetFlagStatus(EVAL_COM2, USART_FLAG_TC) == RESET);
		USART_SendData(EVAL_COM2, *pdata++);
    }

    return ret;
}

static INT32S _conn_read(INT8U* pdata, INT16U datalen)
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

static void _para_replace(INT8U *src, INT16U *srclen, INT8U *cmd, INT8U paranum, void *p)
{
	INT16U i, j, n, len, paralen;
    char (*para)[AT_PARA_SIZE] = (char (*)[AT_PARA_SIZE])p;  

	len = strlen((char*)cmd);
	for (i = 0, j = 0, n = 0; (i < len) && (j+1 < *srclen); i++) {
		if (cmd[i] == '$') {
            if (paranum-- == 0) {
                break;
            }

            paralen = strlen((char*)para[n]);
            if (j + paralen >= *srclen) {
                break;
            }
			memcpy(&src[j], para[n++], paralen);
			j += paralen;
		} else {
            src[j++] = cmd[i];
        }
	}

    src[j] = '\0';
    *srclen = j;
}
 
/*******************************************************************
** 函数名:     _at_handle_tmr
** 函数描述:   at handle定时器
** 参数:       [in] index  : 定时器参数
** 返回:       无
********************************************************************/
static void _at_handle_tmr(void *index)
{	

    os_timer_start(s_at.at_tmr, 1*MILTICK);                                    /* 100ms */
    switch (s_at.at_step) {
        case AT_STEP_SEND:
        {
            INT8U at_cmd_buf[256];
            INT16U cmd_buf_len = sizeof(at_cmd_buf);
            
            if (s_at.at_cmd->paranum > 0) {
                _para_replace(at_cmd_buf, &cmd_buf_len, s_at.at_cmd->cmd, 
                    s_at.at_cmd->paranum, s_at.para);
            } else {
                if (strlen((char*)s_at.at_cmd->cmd) < cmd_buf_len) {
                    cmd_buf_len = strlen((char*)s_at.at_cmd->cmd);
                } else {
                    cmd_buf_len--;                                             /* 字符串结尾必须是\0 */
                }
                
                memcpy(at_cmd_buf, s_at.at_cmd->cmd, cmd_buf_len);
                at_cmd_buf[cmd_buf_len] = '\0';
            }

            SYS_DEBUG("<write at:%s>\n", at_cmd_buf);
            roundbuf_reset(&s_roundbuf);
            _conn_write(at_cmd_buf, cmd_buf_len);
            s_at.ret   = AT_DOING;
            s_at.rflag = 0;
            s_at.rlen  = 0;
            s_at.precv = s_at.rbuf;
            s_at.at_step = AT_STEP_RECV;
            break;
        }
        case AT_STEP_RECV:
        {
            s_at.wait += 100;                                                  /* 等待间隔100ms */
            if (s_at.wait < s_at.at_cmd->timeout) {
                INT16S ret;
                
                if (s_at.rlen >= sizeof(s_at.rbuf)) {
                    goto PARSE;
                }

                ret = _conn_read(s_at.precv, sizeof(s_at.rbuf) - s_at.rlen);
                if (ret > 0) {
                    s_at.rflag = 1;
                    s_at.rlen += ret;
                    s_at.precv += ret;
                } else {
                    if (s_at.rflag) {
                        goto PARSE;
                    }
                }
                break;
            } else {
                if (s_at.rflag) {
                    goto PARSE;
                }

                s_at.ret = AT_TIMEOUT;
                goto END;
            }
PARSE:
            if (s_at.rlen >= sizeof(s_at.rbuf)) {
                s_at.rbuf[sizeof(s_at.rbuf)-1] = '\0';
            } else {
                s_at.rbuf[s_at.rlen] = '\0';
            }
    
            SYS_DEBUG("<read at:[%d]%s>\n", s_at.rlen, s_at.rbuf);

            if (mem_find_str(s_at.rbuf, s_at.rlen, (char*)s_at.at_cmd->scuesscode)) {
                s_at.ret = AT_OK;
            } else {
                s_at.ret = AT_FAIL;
            }
            goto END;
        }
        default:
            s_at.ret = AT_INVALID_PARAMS;
            goto END;
    }
    return;

END:
    if (s_at.fp != NULL) {
        s_at.fp(s_at.ret);
    }
    s_at.at_step = AT_STEP_FREE;
    os_timer_stop(s_at.at_tmr);
}

static void _at_write_and_read(const At_cmd_t *at_cmd, void *para, void (*callback)(At_ret result))
{
    if (at_cmd->paranum > 0) {
        return_if_fail(at_cmd->paranum <= AT_PARA_NUM);
        if (para != NULL) {
            memcpy(s_at.para, para, at_cmd->paranum*AT_PARA_SIZE);
        } else {
            return_if_fail(at_cmd->para != NULL);
            memcpy(s_at.para, at_cmd->para, at_cmd->paranum*AT_PARA_SIZE);
        }
    } else {
        memset(s_at.para, 0, AT_PARA_NUM*AT_PARA_SIZE);
    }

    s_at.at_cmd  = at_cmd;
    s_at.at_step = AT_STEP_SEND;
    s_at.wait    = 0;
    s_at.rflag   = 0;
    s_at.rlen    = 0;
    s_at.precv   = s_at.rbuf;
    s_at.ret     = AT_DOING;
    s_at.fp      = callback;
    
    os_timer_start(s_at.at_tmr, 1*MILTICK);
}

/*******************************************************************
** 函数名:     _link_callback
** 函数描述:   at执行回调函数
** 参数:       [in] result  : 执行结果
** 返回:       无
********************************************************************/
static void _link_callback(At_ret result)
{
    if (result == AT_OK) {
        s_priv.failcnt = 0;
        os_timer_start(s_priv.link_tmr, 1);
    
        switch (s_priv.step) {
            case STEP_CLOSE_TRANS:
            {
                if (s_priv.smartcfg == TRUE) {
                    s_priv.step = STEP_SMART_ON;
                } else {
                    s_priv.step = STEP_SET_STA;
                }    
                break;
            }
            case STEP_SMART_ON:
            {
                s_priv.step = STEP_SMART_WAIT;
                break;
            }
            case STEP_SMART_OFF:
            {
                s_priv.step = STEP_SET_STA;
                break;
            }            
            case STEP_SET_STA:
            {
                s_priv.step = STEP_RST;
                break;
            }
            case STEP_RST:
            {
                s_priv.step = STEP_CLOSE_ATE;
                os_timer_start(s_priv.link_tmr, 2*SECOND);
                break;
            }   
            case STEP_CLOSE_ATE:
            {
                s_priv.step = STEP_JOIN_AP;
                break;
            }
            case STEP_JOIN_AP:
            {
                s_priv.step = STEP_QUERY_AP;
                break;
            }
            case STEP_QUERY_AP:
            {
                if (mem_find_str(s_at.rbuf, s_at.rlen, "No AP")) {
                    ;
                } else if (mem_find_str(s_at.rbuf, s_at.rlen, "+CWJAP:")) { 
                    s_priv.step = STEP_TCP_CONNECT;
                    break;
                } else { 
                    ;
                    /* 如果一直连接不上路由器，可能要重新配置 */
                }
                os_timer_start(s_priv.link_tmr, 10*SECOND);
                s_priv.qjapcnt++;
                if (s_priv.qjapcnt == 12) {
                    s_priv.step = STEP_JOIN_AP;
                } else if (s_priv.qjapcnt == 24) {
                    s_priv.step = STEP_POWER_OFF;
                }
                break;
            }
            case STEP_TCP_CONNECT:
            {
                s_priv.step = STEP_OPEN_TRANS;
                break;
            }
            case STEP_OPEN_TRANS:
            {
                s_priv.step = STEP_TRANS;
                break;
            }
            case STEP_TRANS:
            {
                s_priv.step = STEP_OK;
                break;
            }
            default:
                break;
        }

    } else {
        SYS_DEBUG("<cb fail step:%d, result:%d>\n", s_priv.step, result);

        switch (s_priv.step) {
            case STEP_JOIN_AP:
            {
                s_priv.step = STEP_QUERY_AP;
                os_timer_start(s_priv.link_tmr, 1);
                break;
            }
            case STEP_TCP_CONNECT:
            {
                s_priv.failcnt++;
                os_timer_start(s_priv.link_tmr, s_priv.failcnt*5*SECOND);
                if (s_priv.failcnt > 20) {
                    s_priv.failcnt = 0;
                    s_priv.step = STEP_POWER_OFF;
                }
                
                break;
            }
            default:
            {
                s_priv.failcnt++;

                os_timer_start(s_priv.link_tmr, s_priv.failcnt*5*SECOND);
                if (s_priv.failcnt > 10) {
                    s_priv.failcnt = 0;
                    s_priv.step = STEP_POWER_OFF;
                }
                break;
            }
        }
    }

}

/*******************************************************************
** 函数名:     _link_tmr
** 函数描述:   link定时器
** 参数:       [in] index  : 定时器参数
** 返回:       无
********************************************************************/
static void _link_tmr(void *index)
{
    char para[4][AT_PARA_SIZE];
    index = index;

    switch (s_priv.step) {
        case STEP_POWER_OFF:                                                   /* 断电模块1s */
        {
            SYS_DEBUG("<_link_tmr: POWER OFF>\n");
            /* TO DO 执行power off */

            s_priv.qjapcnt = 0;
            s_priv.step = STEP_POWER_ON;
            os_timer_start(s_priv.link_tmr, 1*SECOND);
            break;
        }
        case STEP_POWER_ON:                                                    /* 上电延时1s */
        {
            SYS_DEBUG("<_link_tmr: POWER ON>\n");
            
            /* TO DO 执行power on */

            s_priv.step = STEP_QUIT_TRANS;
            os_timer_start(s_priv.link_tmr, 1*SECOND);
            break;
        }
        case STEP_QUIT_TRANS:                                                  /* 退出透传模式 */
        {
            SYS_DEBUG("<_link_tmr: QUIT_TRANS>\n");
            
            _conn_write((INT8U*)"+++", 3);

            s_priv.step = STEP_CLOSE_TRANS;
            os_timer_start(s_priv.link_tmr, 5*MILTICK);
            break;
        }
        case STEP_CLOSE_TRANS:                                                 /* 关闭透传模式 */
        {
            SYS_DEBUG("<_link_tmr: CLOSE_TRANS>\n");

            sprintf(para[0], "%d", 0);
            _at_write_and_read(&c_at_cmd_sets[ATCIPMODE], para, _link_callback);

            os_timer_stop(s_priv.link_tmr);
            break;
        }    
        case STEP_SMART_ON:                                                    /* 开启smartcfg */
        {
            SYS_DEBUG("<_link_tmr: SMART_ON>\n");

            _at_write_and_read(&c_at_cmd_sets[ATSMART_ON], NULL, _link_callback);

            os_timer_stop(s_priv.link_tmr);
            break;
        }
        case STEP_SMART_WAIT:                                                  /* 等待smartcfg */
        {
            INT8U i, *ptr;
            INT32S ret;
            static INT8U wait =0;
            
            SYS_DEBUG("<_link_tmr: SMART_WAIT>\n");
            os_timer_start(s_priv.link_tmr, 3*SECOND);

            ret = _conn_read(s_at.rbuf, sizeof(s_at.rbuf));
            if (ret > 0) {
                if ((ptr = mem_find_str(s_at.rbuf, ret, "SSID:")) == NULL) {
                    goto ERR;
                }

                ptr += strlen("SSID:");
                for (i = 0; i < sizeof(s_priv.para.ssid) - 1; i++) {
                    if (*ptr == '\r' || *ptr == '\n') {
                        break;
                    }
                    s_priv.para.ssid[i] = *ptr++;
                }

                s_priv.para.ssid[i] = '\0';

                if ((ptr = mem_find_str(s_at.rbuf, ret, "PASSWORD:")) == NULL) {
                    goto ERR;
                }

                ptr += strlen("PASSWORD:");
                for (i = 0; i < sizeof(s_priv.para.pwd) - 1; i++) {
                    if (*ptr == '\r' || *ptr == '\n') {
                        break;
                    }
                    s_priv.para.pwd[i] = *ptr++;
                }
                
                s_priv.para.pwd[i] = '\0';

                public_para_manager_store_by_id(WIFI_PARA_, (INT8U*)&s_priv.para, sizeof(Wifi_para_t));        
ERR:
                wait = 0;
                s_priv.smartcfg = 0;
                public_para_manager_read_by_id(WIFI_PARA_, (INT8U*)&s_priv.para, sizeof(Wifi_para_t));        
                s_priv.step = STEP_SMART_OFF;
                os_timer_start(s_priv.link_tmr, 5*SECOND);
            } else {
                if (wait++ > 20) {
                    wait = 0;
                    s_priv.smartcfg = 0;
                    s_priv.step = STEP_SMART_OFF;
                    os_timer_start(s_priv.link_tmr, 5*SECOND);
                }
            }
            break;
        }        
        case STEP_SMART_OFF:                                                   /* 关闭smartcfg */
        {
            SYS_DEBUG("<_link_tmr: SMART_OFF>\n");

            _at_write_and_read(&c_at_cmd_sets[ATSMART_OFF], NULL, _link_callback);

            os_timer_stop(s_priv.link_tmr);
            break;
        }    
        case STEP_SET_STA:                                                     /* 设置为STA模式 */
        {
            SYS_DEBUG("<_link_tmr: SET_STA>\n");
            sprintf(para[0], "%d", 1);
            _at_write_and_read(&c_at_cmd_sets[ATCWMODE], para, _link_callback);

            os_timer_stop(s_priv.link_tmr);
            break;
        }   
        case STEP_RST:                                                         /* 重启 */
        {
            SYS_DEBUG("<_link_tmr: RST>\n");
            _at_write_and_read(&c_at_cmd_sets[ATRST], NULL, _link_callback);

            os_timer_stop(s_priv.link_tmr);
            break;
        }
        case STEP_CLOSE_ATE:                                                   /* 关闭回显 */
        {
            SYS_DEBUG("<_link_tmr: CLOSE_ATE>\n");
            _at_write_and_read(&c_at_cmd_sets[ATE], NULL, _link_callback);

            os_timer_stop(s_priv.link_tmr);
            break;
        }  
        case STEP_JOIN_AP:                                                     /* 加入AP */
        {
            SYS_DEBUG("<_link_tmr: JOIN_AP>\n");
            snprintf(para[0], AT_PARA_SIZE, "%s", s_priv.para.ssid);
            snprintf(para[1], AT_PARA_SIZE, "%s", s_priv.para.pwd);
            
            _at_write_and_read(&c_at_cmd_sets[ATCWJAP], para, _link_callback);

            os_timer_stop(s_priv.link_tmr);
            break;
        }
        case STEP_QUERY_AP:                                                    /* 查询AP状态 */
        {
            SYS_DEBUG("<_link_tmr: QUERY_AP>\n");

            _at_write_and_read(&c_at_cmd_sets[ATCWJAP_Q], NULL, _link_callback);

            os_timer_stop(s_priv.link_tmr);
            break;
        }
        case STEP_TCP_CONNECT:                                                 /* TCP 连接 */
        {
            SYS_DEBUG("<_link_tmr: TCP_CONNECT>\n");
            strncpy(para[0], modetbl[0], AT_PARA_SIZE); 
            strncpy(para[1], s_priv.para.server, AT_PARA_SIZE);
            sprintf(para[2], "%d", s_priv.para.port);
            _at_write_and_read(&c_at_cmd_sets[ATCIPSTART], para, _link_callback);

            os_timer_stop(s_priv.link_tmr);
            break;
        }
        case STEP_OPEN_TRANS:                                                  /* 打开透传模式 */
        {
            SYS_DEBUG("<_link_tmr: OPEN_TRANS>\n");

            sprintf(para[0], "%d", 1);
            _at_write_and_read(&c_at_cmd_sets[ATCIPMODE], para, _link_callback);

            os_timer_stop(s_priv.link_tmr);
            break;
        }
        case STEP_TRANS:                                                       /* 开始透传 */
        {
            SYS_DEBUG("<_link_tmr: TRANS>\n");

            _at_write_and_read(&c_at_cmd_sets[ATCIPSEND], NULL, _link_callback);

            os_timer_stop(s_priv.link_tmr);
            break;
        }        
        case STEP_OK:                                                          /* 成功 */
        {
            SYS_DEBUG("<_link_tmr: OK>\n");


            os_timer_stop(s_priv.link_tmr);
            break;
        }      
        default:
            break;
    }
}

/*******************************************************************
** 函数名:      drv_esp8266_init
** 函数描述:    
** 参数:        none
** 返回:        none
********************************************************************/
void drv_esp8266_init(void)
{
    memset(&s_priv, 0, sizeof(Priv_t));

    s_at.at_tmr = os_timer_create(0, _at_handle_tmr);
    
    s_priv.link_tmr  = os_timer_create(0, _link_tmr);
    os_timer_start(s_priv.link_tmr, 1*SECOND);

    roundbuf_init(&s_roundbuf, s_recv_buf, sizeof(s_recv_buf), NULL); 
    /* bug:(可能存在bug)最好在循环缓冲区初始化后，在开启usart2中断 */
    usart_rxit_enable(COM2);
    
    public_para_manager_read_by_id(WIFI_PARA_, (INT8U*)&s_priv.para, sizeof(Wifi_para_t));        
}

INT32S drv_esp8266_write(INT8U* pdata, INT16U datalen)
{
    if (s_priv.step != STEP_OK) {
        return -1;
    }
    
    return _conn_write(pdata, datalen);
}

INT32S drv_esp8266_read(INT8U* pdata, INT16U datalen)
{
    if (s_priv.step != STEP_OK) {
        return -1;
    }

    return _conn_read(pdata, datalen);
}

BOOLEAN drv_esp8266_is_connect(void)
{
    if (s_priv.step == STEP_OK) {
        return true;
    } else {
        return false;
    }
}

BOOLEAN drv_esp8266_reset(INT8U delay)
{
    s_priv.step = STEP_POWER_OFF;
    s_priv.failcnt = 0;

    os_timer_stop(s_at.at_tmr);

    os_timer_start(s_priv.link_tmr, 1*delay);
    return true;
}

BOOLEAN drv_esp8266_smartconfig(void)
{
    s_priv.smartcfg = TRUE;

    drv_esp8266_reset(1);

    return true;
}

BOOLEAN drv_esp8266_server(char *server)
{
    snprintf(s_priv.para.server, sizeof(s_priv.para.server), "%s", server);
    public_para_manager_store_by_id(WIFI_PARA_, (INT8U*)&s_priv.para, sizeof(Wifi_para_t));        

    drv_esp8266_reset(30);
    return true;
}

BOOLEAN drv_esp8266_port(INT16U port)
{
    s_priv.para.port = port;
    public_para_manager_store_by_id(WIFI_PARA_, (INT8U*)&s_priv.para, sizeof(Wifi_para_t));        

    drv_esp8266_reset(30);
    return true;
}

#endif /* end of EN_WIFI */

