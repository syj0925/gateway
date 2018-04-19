/********************************************************************************
**
** 文件名:     debug_cmd.c
** 版权所有:   
** 文件描述:   接收解析debug串口命令
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**|    日期    |  作者  |  修改记录
**===============================================================================
**| 2015/06/12 | 苏友江 |  创建该文件
********************************************************************************/
#include "bsp.h"
#include "sys_includes.h"

#if DBG_SYSTEM > 0
#define SYS_DEBUG          OS_DEBUG
#else
#define SYS_DEBUG(...)     do{}while(0)
#endif

/*
********************************************************************************
* 定义模块结构
********************************************************************************
*/
/* 用于识别主动发送的字符串 */
typedef enum {
    REBOOT,                 /* 终端重启 */
    RESTORE,                /* 恢复出厂设置 */
    SMARTCFG,               /* WIFI智能配置 */
    RESETWIFI,              /* 重启wifi模块 */
    SERVER,                 /* 服务器ip */
    PORT,                   /* 服务器端口 */
    NRFCH,                  /* nRF24L01信道设置 */
    NRFPWR,                 /* nRF24L01功率设置 */
    STR_TAB_MAX
} String_tab_e;  

/*
********************************************************************************
* 定义静态变量
********************************************************************************
*/
const String_tab_t string_tab[STR_TAB_MAX] = {
    {REBOOT,     6,   "reboot"         },
    {RESTORE,    7,   "restore"        },
    {SMARTCFG,   8,   "smartcfg"       },
    {RESETWIFI,  9,   "resetwifi"      },
    {SERVER,     6,   "server"         },
    {PORT,       4,   "port"           },
    {NRFCH,      5,   "nrfch"          },
    {NRFPWR,     6,   "nrfpwr"         },
    
};

String_ident_t s_ident;

static INT8U s_recv_buf[60];
static RoundBuf_t s_roundbuf = {0};
static BOOLEAN s_cmd_mode = 0;                                                 /* 1为命令模式 */
static INT32U s_delay = 0;
static INT8U s_cmdtmr;


void USART1_IRQHandler(void)
{
    const char cmd[] = "cmd\r";
    static INT8U idx = 0;
    
	if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        char ch;
        
        ch = (USART_ReceiveData(USART1) & 0xFF);
        printf("%c", ch);

        if (s_cmd_mode == 0) {
            if (cmd[idx] == ch) {
                idx++;
                if (idx == sizeof(cmd) - 1) {
                    idx = 0;
                    s_cmd_mode = 1;
                    g_debug_sw = 0;
                    s_delay = g_run_sec;
                    os_timer_start(s_cmdtmr, 1);
                    printf("\r\n---Enter cmd!!!---\n");
                }
            } else {
                idx = 0;
            }
        } else {
    		roundbuf_write_byte(&s_roundbuf, ch);
        }
	}
}

static void _cmd_handle_tmr(void *index)
{
    static INT8U para[30], paralen = 0;
    INT8U readbuf[50], *p = readbuf;
    INT32U readlen;
    
    readlen = roundbuf_read_data(&s_roundbuf, readbuf, sizeof(readbuf));
    
    if (readlen <= 0) {
        if (g_run_sec - s_delay >= 30) {
            s_cmd_mode = 0;
            g_debug_sw = 1;
            paralen    = 0;
            s_ident.tabid = STR_TAB_MAX;
            os_timer_stop(s_cmdtmr);
        }
        return;
    }

    g_debug_sw = 0;
    s_delay = g_run_sec;
ENTRY:

    switch (s_ident.tabid) {
        case REBOOT:
            s_ident.tabid = STR_TAB_MAX;
            os_msg_post(TSK_SYS_MSG, MSG_PP_RESET, 0, 0, NULL);
            break;
        case RESTORE:
            s_ident.tabid = STR_TAB_MAX;
            os_msg_post(TSK_SYS_MSG, MSG_PP_RESTOREFACTORY, 0, 0, NULL);
            break;
        case SMARTCFG:
            s_ident.tabid = STR_TAB_MAX;
            #if EN_WIFI > 0
            drv_esp8266_smartconfig();
            #endif
            g_debug_sw = 1;
            break;
        case RESETWIFI:
            s_ident.tabid = STR_TAB_MAX;
            #if EN_WIFI > 0
            drv_esp8266_reset(1);
            #endif
            g_debug_sw = 1;
            break;
        case SERVER:
        case PORT:
        case NRFCH:
        case NRFPWR:
            
            if (paralen + readlen <= sizeof(para)) {
                memcpy(para+paralen, p, readlen);
                paralen += readlen;
            } else {
                s_ident.tabid = STR_TAB_MAX;
                memcpy(para+paralen, p, sizeof(para) - paralen);
                paralen = sizeof(para);
            }
            
            if (mem_find_str(para, paralen, "\r")) {
                
                if (s_ident.tabid == SERVER) {
                    printf("<---ip set ok>\n");
                    para[paralen-1] = '\0';
                    #if EN_WIFI > 0
                    drv_esp8266_server((char*)para);
                    #endif
                } else if (s_ident.tabid == PORT) {
                    INT32S port;

                    printf("<---port set ok>\n");
                    para[paralen-1] = '\0';
                    if (sscanf((char*)para, "%d", &port) == 1) {
                        #if EN_WIFI > 0
                        drv_esp8266_port(port);
                        #endif
                    }
                } else if (s_ident.tabid == NRFCH) {
                    INT32S ch;

                    printf("<---nrfch set ok>\n");
                    para[paralen-1] = '\0';
                    if (sscanf((char*)para, "%d", &ch) == 1) {
                        #if EN_NRF24L01 > 0
                        Rf_para_t rf_para;

                        public_para_manager_read_by_id(RF_PARA_, (INT8U*)&rf_para, sizeof(Rf_para_t));
                        rf_para.rf_ch = ch;
                        public_para_manager_store_by_id(RF_PARA_, (INT8U*)&rf_para, sizeof(Rf_para_t));
                        #endif
                        g_debug_sw = 1;
                    }

                } else if (s_ident.tabid == NRFPWR) {
                    INT32S pwr;

                    printf("<---nrfpwr set ok>\n");
                    para[paralen-1] = '\0';
                    if (sscanf((char*)para, "%x", &pwr) == 1) {
                        #if EN_NRF24L01 > 0
                        Rf_para_t rf_para;

                        public_para_manager_read_by_id(RF_PARA_, (INT8U*)&rf_para, sizeof(Rf_para_t));
                        rf_para.rf_pwr = pwr;
                        public_para_manager_store_by_id(RF_PARA_, (INT8U*)&rf_para, sizeof(Rf_para_t));
                        #endif
                        g_debug_sw = 1;
                    }
                }
                    
                s_ident.tabid = STR_TAB_MAX;
            }

            if (s_ident.tabid == STR_TAB_MAX) {
                paralen = 0;
            }
            readlen = 0;
            break;            
        default:
            if (string_ident(&s_ident, &p, &readlen) >= 0) {
                printf("<---ident:%s>\n", s_ident.tab[s_ident.tabid].name);
                goto ENTRY;
            }
            break;
    }

    if (readlen > 0) {
        goto ENTRY;
    }
}

/*******************************************************************
** 函数名:      debug_cmd_init
** 函数描述:    debug cmd初始化
** 参数:        无
** 返回:        无
********************************************************************/
void debug_cmd_init(void)
{
    memset(&s_ident, 0, sizeof(String_ident_t));
    s_ident.tab    = string_tab;
    s_ident.tabid  = STR_TAB_MAX;
    s_ident.tabnum = STR_TAB_MAX;

    s_cmdtmr = os_timer_create(0, _cmd_handle_tmr);

    roundbuf_init(&s_roundbuf, s_recv_buf, sizeof(s_recv_buf), NULL); 
    usart_rxit_enable(COM1);
}

