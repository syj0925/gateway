/********************************************************************************
**
** 文件名:     wsn_sensor.c
** 版权所有:   (c) 2013-2017
** 文件描述:   无线传感器网络
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**|    日期    |  作者  |  修改记录
**===============================================================================
**| 2017/8/4 | 苏友江 |  创建该文件
********************************************************************************/
#include "bsp.h"
#include "sys_includes.h"

#if DBG_GENERAL > 0
#define SYS_DEBUG          OS_DEBUG
#else
#define SYS_DEBUG(...)     do{}while(0)
#endif
/*
********************************************************************************
* 参数配置
********************************************************************************
*/




/*
********************************************************************************
* 结构定义
********************************************************************************
*/


/*
********************************************************************************
* 静态变量
********************************************************************************
*/
#define DATA_BUF_SIZE    512

static INT8U s_buf[DATA_BUF_SIZE];

#define API_KEY          "49a79746d6be66694e6c6963dea1d4cc"
#define REMOTE_SERVER    "api.yeelink.net"


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

static void _sensor_report(PACKET_BUF_T *packet)
{
    float temperature;
	char value[16]={0};
    INT16U len;
    Net_link_node_t *node;

    node = packet->node;

#if 0
    if (node->addr[0] == 0x26) {
        temperature = (float)node->sensor.temperature/10;
        printf("<----addr:%d--_temp:%f>\n", node->addr[0], temperature);

        sprintf(value, "%f", temperature);        
        len = _asm_http_post((char *)s_buf, DATA_BUF_SIZE, "148459", "411009", value);
    } else if (node->addr[0] == 0x2d) {
        temperature = (float)node->sensor.temperature/10;
        sprintf(value, "%f", temperature);  
    
        len = _asm_http_post((char *)s_buf, DATA_BUF_SIZE, "148459", "411010", value);
    }  else if (node->addr[0] == 0x7e) {
        sprintf(value, "%d", node->sensor.status.b.rain);  
        len = _asm_http_post((char *)s_buf, DATA_BUF_SIZE, "148459", "411040", value);
    }  else if (node->addr[0] == 0xbd) {
        sprintf(value, "%d", node->sensor.status.b.motion);  
        len = _asm_http_post((char *)s_buf, DATA_BUF_SIZE, "148459", "411041", value);
    }
#endif /* if 0. 2017-8-10 15:22:08 syj */
    if (node->addr[0] == 0x26) {
        temperature = (float)node->sensor.temperature/10;
        len = sprintf((char *)s_buf, "地址26温度:%f度", temperature);        
    } else if (node->addr[0] == 0x2d) {
#if 0
        temperature = (float)node->sensor.temperature/10;
        len = sprintf((char *)s_buf, "地址2d温度:%f度", temperature); 
#endif /* if 0. 2017-8-24 17:26:20 syj */
        if (node->sensor.status.b.mq2) {
            len = sprintf((char *)s_buf, "地址2d烟雾报警");
            tts_play(7, 1);
        } else {
            len = sprintf((char *)s_buf, "地址2d烟雾报警解除");  
            tts_play_stop(3);
            tts_play(8, 0);
        }
    }  else if (node->addr[0] == 0x7e) {
        if (node->sensor.status.b.rain) {
            len = sprintf((char *)s_buf, "地址7e检测到下雨");
            tts_play(3, 1);
        } else {
            len = sprintf((char *)s_buf, "地址7e检测到晴天");  
            tts_play_stop(3);
            tts_play(4, 0);
        }
    }  else if (node->addr[0] == 0xbd) {
        if (node->sensor.status.b.motion) {
            len = sprintf((char *)s_buf, "地址bd检测到有人");   
            tts_play(1, 0);
        } else {
            len = sprintf((char *)s_buf, "地址bd检测到没人");  
            tts_play(2, 0);
        }
    }

#if EN_WIFI > 0
    drv_esp8266_write(s_buf, len);
#endif
}

/*******************************************************************
** 函数名:      _ack_0x03
** 函数描述:    0X03请求应答
** 参数:        [in] node  : 节点
**              [in] result: 应答结果
** 返回:        无
********************************************************************/
static void _ack_0x03(PACKET_BUF_T *packet, INT8U result)
{
    Stream_t wstrm;

    stream_init(&wstrm, packet->pdata, PLOAD_WIDTH-LINK_HEAD_LEN);
    stream_write_byte(&wstrm, result);
    net_link_send_dirsend(0x83, packet);
}

/*******************************************************************
** 函数名:     _hdl_0x03
** 函数描述:   传感器数据上报 
** 参数:       [in]cmd    : 命令编码
**             [in]packet : 数据指针
** 返回:       无
********************************************************************/
static void _hdl_0x03(INT8U cmd, PACKET_BUF_T *packet)
{
    BOOLEAN ack;
    INT8U type;
    Stream_t rstrm;
    Net_link_node_t *node;

    node = packet->node;
    stream_init(&rstrm, packet->pdata, packet->len);

    ack = stream_read_byte(&rstrm);

    if (ack) {                                                                 /* 需要应答 */
        _ack_0x03(packet, _SUCCESS);
    } else {
        #if SEND_MODE == PASSIVE_SEND
        net_link_send_inform(packet->node);                                    /* 对于直连主节点的终端节点，由于终端节点只有发送完数据后的一段时间内处于接收状态(降低功耗)，因此下发的数据只能在收到心跳数据后发送 */
        #endif
    }

    while (stream_get_left_len(&rstrm) > 0) {
        type = stream_read_byte(&rstrm);
        
        switch(type) {
            case 0x01:
                node->sensor.status.b8 = stream_read_byte(&rstrm);             /* 警报状态 */
                break;
            case 0x02:
                node->sensor.temperature = stream_read_half_word(&rstrm);      /* 温度 */
                break;
            default:
                goto EXIT;
        }
    }

EXIT:
    _sensor_report(packet);
}  

/*******************************************************************
** 函数名:     _hdl_0x05
** 函数描述:   S1按键上报
** 参数:       [in]cmd    : 命令编码
**             [in]packet : 数据指针
** 返回:       无
********************************************************************/
static void _hdl_0x05(INT8U cmd, PACKET_BUF_T *packet)
{
    #if SEND_MODE == PASSIVE_SEND
    net_link_send_inform(packet->node);                                        /* 对于直连主节点的终端节点，由于终端节点只有发送完数据后的一段时间内处于接收状态(降低功耗)，因此下发的数据只能在收到心跳数据后发送 */
    #endif
    
    tts_play_stop(3);
    tts_play(5, 0);
}

/*******************************************************************
** 函数名:     _hdl_0x06
** 函数描述:   S2按键上报
** 参数:       [in]cmd    : 命令编码
**             [in]packet : 数据指针
** 返回:       无
********************************************************************/
static void _hdl_0x06(INT8U cmd, PACKET_BUF_T *packet)
{
    #if SEND_MODE == PASSIVE_SEND
    net_link_send_inform(packet->node);                                        /* 对于直连主节点的终端节点，由于终端节点只有发送完数据后的一段时间内处于接收状态(降低功耗)，因此下发的数据只能在收到心跳数据后发送 */
    #endif
    
    tts_play(6, 0);
}

/*
********************************************************************************
* 注册回调函数
********************************************************************************
*/
static FUNCENTRY_LINK_T s_functionentry[] = {
        0x03,                   _hdl_0x03,
        0x05,                   _hdl_0x05,
        0x06,                   _hdl_0x06
};
static INT8U s_funnum = sizeof(s_functionentry) / sizeof(s_functionentry[0]);

/*******************************************************************
** 函数名:      wsn_sensor_init
** 函数描述:    传感器采集初始化
** 参数:        无
** 返回:        无
********************************************************************/
void wsn_sensor_init(void)
{
    INT8U i;

    for (i = 0; i < s_funnum; i++) {
        net_link_register(s_functionentry[i].index, s_functionentry[i].entryproc);
    }
}

