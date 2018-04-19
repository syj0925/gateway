/********************************************************************************
**
** 文件名:     yeelink.c
** 版权所有:   (c) 2013-2015
** 文件描述:   连接yeelink平台，演示开关灯操作等
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**|    日期    |  作者  |  修改记录
**===============================================================================
**| 2015/08/29 | 苏友江 |  创建该文件
********************************************************************************/
#include "bsp.h"
#include "sys_includes.h"
#if EN_YEELINK > 0

#if DBG_YEELINK > 0
#define SYS_DEBUG          OS_DEBUG
#else
#define SYS_DEBUG(...)     do{}while(0)
#endif


#define API_KEY          "49a79746d6be66694e6c6963dea1d4cc"
#define REMOTE_SERVER    "api.yeelink.net"
#define DATA_BUF_SIZE    512

static INT8U s_yeelinktmr;
static INT8U s_buf[DATA_BUF_SIZE];

static INT16U _asm_http_request(char *pbuf, INT16U buflen, const char *device_id, const char *sensors_id)
{
	char str_tmp[128] = {0};

	sprintf(str_tmp, "/v1.0/device/%s/sensor/%s/datapoints", device_id, sensors_id);
	// 确定 HTTP请求首部
	// 例如POST /v1.0/device/98d19569e0474e9abf6f075b8b5876b9/1/1/datapoints/add HTTP/1.1\r\n
	sprintf(pbuf , "GET %s HTTP/1.1\r\n", str_tmp);
	// 增加属性 例如 Host: api.machtalk.net\r\n
	sprintf(str_tmp, "Host:%s\r\n", REMOTE_SERVER);
	strcat(pbuf, str_tmp);

	// 增加密码
	sprintf(str_tmp, "U-ApiKey:%s\r\n", API_KEY);
	strcat(pbuf, str_tmp);

	strcat(pbuf, "Accept: */*\r\n");
	// 增加表单编码格式 Content-Type:application/x-www-form-urlencoded\r\n
	strcat(pbuf, "Content-Type: application/x-www-form-urlencoded\r\n");

    //strcat(pbuf, "Connection: keep-alive\r\n");
	strcat(pbuf, "Connection: close\r\n");
	// HTTP首部和HTTP内容 分隔部分
	strcat(pbuf, "\r\n");

    return strlen(pbuf);
}

static INT16U _asm_http_post(char *pbuf, INT16U buflen, const char *device_id, const char *sensors_id, const char *value)
{
	char str_tmp[128] = {0};
    char http_content[32] = {0};                                               /* Http内容，表单内容 */
    
    sprintf(str_tmp, "/v1.0/device/%s/sensor/%s/datapoints", device_id, sensors_id);
    // 确定HTTP表单提交内容 {"value":20}
    sprintf(http_content, "{\"value\":%s}", value);
    // 确定 HTTP请求首部
    // 例如POST /v1.0/device/98d19569e0474e9abf6f075b8b5876b9/1/1/datapoints/add HTTP/1.1\r\n
    sprintf(pbuf, "POST %s HTTP/1.1\r\n", str_tmp);
    // 增加属性 例如 Host: api.machtalk.net\r\n
    sprintf(str_tmp, "Host:%s\r\n", REMOTE_SERVER);
    strcat(pbuf, str_tmp);

    // 增加密码
    sprintf(str_tmp, "U-ApiKey:%s\r\n", API_KEY);
    strcat(pbuf, str_tmp);

    strcat(pbuf, "Accept: */*\r\n");
    // 增加提交表单内容的长度 例如 Content-Length:12\r\n
    sprintf(str_tmp, "Content-Length:%d\r\n", strlen(http_content));
    strcat(pbuf, str_tmp);
    // 增加表单编码格式 Content-Type:application/x-www-form-urlencoded\r\n
    strcat(pbuf, "Content-Type: application/x-www-form-urlencoded\r\n");
    strcat(pbuf, "Connection: close\r\n");
    // HTTP首部和HTTP内容 分隔部分
    strcat(pbuf, "\r\n");
    // HTTP负载内容
    strcat(pbuf, http_content);

    return strlen(pbuf);
}

#if EN_ETHERNET > 0    
#define SOCK_TCPC        	4

static INT8U domain_ip[4] = {42, 96, 164, 52};
static INT8U s_ch_status[8] = {0}; /* 0:close, 1:ready, 2:connected */
static INT32U s_lasttime[8] = {0};

static void _link_manager(INT8U sn)
{
    switch (getSn_SR(sn))
    {
        case SOCK_ESTABLISHED:                                                 /* if connection is established */
            if (s_ch_status[sn] != 2) {
                s_ch_status[sn] = 2;
                SYS_DEBUG("<socket:%d: Connected>\r\n", sn);
            }            
            break;
        case SOCK_CLOSE_WAIT:                                                  /* If the client request to close */
            SYS_DEBUG("<socket:%d : CLOSE_WAIT>\r\n", sn);
            s_ch_status[sn] = 0;
            disconnect(sn);
            break;
        case SOCK_CLOSED:                                                      /* if a socket is closed */
        {
            if (g_run_sec - s_lasttime[sn] >= 2) {
                SYS_DEBUG("<socket:%d : TCP Client Started. port: %d>\r\n", sn, 80);

                if(socket(sn, Sn_MR_TCP, 5000, 0x00) != sn)                    /* reinitialize the socket */
                {
                    s_ch_status[sn] = 0;
                    SYS_DEBUG("<socket:%d : Fail to create socket.>\r\n", sn);
                } else {
                    s_ch_status[sn] = 1;
                }

                s_lasttime[sn] = g_run_sec;
            }
            break;
        }
        case SOCK_INIT:     /* if a socket is initiated */
        {
            if (g_run_sec - s_lasttime[sn] >= 2) {
                SYS_DEBUG("<socket:%d : connect socket.>\r\n", sn);
                connect(sn, domain_ip, 80);
                s_lasttime[sn] = g_run_sec;
            }
            break;
        }     
        default:
            break;
    }
}
#endif

static void _yeelink_tmr(void *index)
{    
    static INT32U linktime = 0;
    static INT8U loop = 0;
    INT16U len;
    INT32S ret;
	char value[16]={0};
    Lamp_event_t event;

    index = index;

    os_timer_start(s_yeelinktmr, 1*MILTICK);

#if EN_ETHERNET > 0    
    if ((ret = getSn_RX_RSR(SOCK_TCPC)) > 0) {
        if (ret >= DATA_BUF_SIZE) {
            ret = DATA_BUF_SIZE - 1;
        }

        if ((ret = recv(SOCK_TCPC, s_buf, ret)) <= 0) {
            SYS_DEBUG("recv no recv:%d\n", ret);
        }
    }
#endif

#if EN_WIFI > 0
    ret = drv_esp8266_read(s_buf, DATA_BUF_SIZE - 1);
#endif
    
    if (ret > 0) {
        SYS_DEBUG("recv:%d\n", ret);
		SYS_DEBUG("%s", s_buf);

    	s_buf[ret] = '\0';
    	//判断是否收到HTTP OK
    	if (strstr((const char *)s_buf , (const char *)"200 OK\r\n") != NULL) {
            static INT8U ctl = 0xff;
            
    		//提取返回信息
    		//char timestamp[64]={0};
    		char timestampTmp[64]={0};
    		char valueTmp[64]={0};
    		static char strTmp[DATA_BUF_SIZE] = {0};//声明为静态变量，防止堆栈溢出

    		sscanf((const char *)s_buf, "%*[^{]{%[^}]", strTmp);
    		sscanf(strTmp, "%[^,],%[^,]", timestampTmp, valueTmp);
            
    		//strncpy(timestamp, strstr(timestampTmp,":")+2, strlen(strstr(timestampTmp,":"))-3);
    		strncpy(value, strstr(valueTmp, ":")+1, strlen(strstr(valueTmp, ":"))-1);
        	SYS_DEBUG("---%s\n", value);

            event.event = LAMP_EVENT_ALARM;
            if (value[0] == '0') {
                event.ctl = LAMP_CLOSE;
            } else if (value[0] == '1') {
                event.ctl = LAMP_OPEN;
            } else {
                event.ctl = ctl;
            }

            if (ctl != event.ctl) {
                ctl = event.ctl;
                lamp_event_create(LAMP1_INDEX, &event);
            }
            
    	} else {
    		SYS_DEBUG("Http Response Error\r\n");
    		//SYS_DEBUG("%s", s_buf);
    	}
        //close(SOCK_TCPC);
    }        

    if (g_run_sec < linktime) {                                                /* yeelink有最短连接间隔限制，如果时间太短，会导致请求不到server的应答 */
        return;
    }

#if EN_ETHERNET > 0    
    _link_manager(SOCK_TCPC);
    if (s_ch_status[SOCK_TCPC] != 2) {
        return;
    }
#endif

#if EN_WIFI > 0
    if (!drv_esp8266_is_connect()) {
        return;
    }
#endif

    loop++;
    if (loop < 2) {
        len = _asm_http_request((char *)s_buf, DATA_BUF_SIZE, "148459", "170027");
    } else if (loop == 2) {
        sprintf(value, "%f", g_power.lamp[LAMP1_INDEX].p_avr);
        len = _asm_http_post((char *)s_buf, DATA_BUF_SIZE, "148459", "377700", value);
    } else if (loop == 3) {
        sprintf(value, "%f", g_power.lamp[LAMP1_INDEX].i_eff_avr);
        len = _asm_http_post((char *)s_buf, DATA_BUF_SIZE, "148459", "383548", value);
    } else if (loop == 4) {
        loop = 0;
        sprintf(value, "%f", g_power.v_eff_avr);
        len = _asm_http_post((char *)s_buf, DATA_BUF_SIZE, "148459", "383547", value);
    } else {
        loop = 0;
        len = 0;
    }
    
    SYS_DEBUG("_asm_http:%d,%s", len, s_buf);
    
#if EN_ETHERNET > 0
    send(SOCK_TCPC, s_buf, len);
#endif

#if EN_WIFI > 0
    drv_esp8266_write(s_buf, len);
#endif

    linktime = g_run_sec + 10;
}

void yeelink_init(void)
{
    s_yeelinktmr = os_timer_create(0, _yeelink_tmr);
    os_timer_start(s_yeelinktmr, 10*SECOND);
}

#endif /* end of EN_YEELINK */

