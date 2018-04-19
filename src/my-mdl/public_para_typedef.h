/********************************************************************************
**
** 文件名:     public_para_typedef.h
** 版权所有:   (c) 2013-2015 
** 文件描述:   实现公共参数结构体定义和参数定义
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**|    日期    |  作者  |  修改记录
**===============================================================================
**| 2015/08/28 | 苏友江 |  创建该文件
********************************************************************************/
#ifndef PUBLIC_PARA_TYPEDEF_H
#define PUBLIC_PARA_TYPEDEF_H   1

typedef enum {
    LAMP1_INDEX = 0,
    LAMP_NUM_MAX
} Lamp_e;

/*
********************************************************************************
* 系统信息结构定义
********************************************************************************
*/
/*
********************************************************************************
* 系统信息结构定义
********************************************************************************
*/
#define GROUP_NUM   3

typedef struct {
    INT32U uid;               /* 终端uid */
    INT8U group[GROUP_NUM];   /* 终端分组 */
} Sys_base_info_t;            /* 对于一些很少修改，并且很重要的系统信息 */

typedef struct {
    INT32U restartcnt;        /* 终端重启次数 */
    INT32U wdgcnt;            /* 看门狗复位次数 */
    
} Sys_run_info_t;



/*
********************************************************************************
* 灯控制事件结构定义
********************************************************************************
*/
typedef enum {
    LAMP_EVENT_ALARM,
    LAMP_EVENT_CENTER,
    LAMP_EVENT_AUTO,
    LAMP_EVENT_MAX
} Lamp_envnt_e;

typedef enum {
    LAMP_CLOSE,
    LAMP_OPEN
} Lmap_ctl_e;


typedef struct {
    INT8U event;
    INT8U ctl;
    INT8U dimming;
} Lamp_event_t;

/*
********************************************************************************
* 存储电能参数
********************************************************************************
*/
typedef struct {
	float  lamp_ws[LAMP_NUM_MAX];           /* 瓦秒 */
    INT32U t_light[LAMP_NUM_MAX];           /* 亮灯时长 */
} Power_total_t;

/*
********************************************************************************
* 网络参数
********************************************************************************
*/
typedef struct { 
    INT8U   mac[6];                         /* Source Mac Address */  
    INT8U   ip[4];                          /* Source IP Address */ 
    INT8U   sn[4];                          /* Subnet Mask */ 
    INT8U   gw[4];                          /* Gateway IP Address */
    INT8U   dns[4];                         /* DNS server IP Address */ 
    INT8U   dhcp;                           /* 1 - Static, 2 - DHCP */ 
    INT8U   server_ip[4];                   /* Dest IP Address */ 
    INT16U  server_port;                    /* Dest port */ 
} Ip_para_t;

typedef struct {
    char    ssid[30];                       /* 接入点名称 */
    char    pwd[30];                        /* 密码 */
    char    server[30];                     /* ip or domain name */
    INT16U  port;                           /* Dest port */     
} Wifi_para_t;

/* 针对nRF24L01 */
typedef struct {
    INT8U rf_ch;                            /* 6:0，0000010，设置芯片工作时的信道，分别对应1~125 个道,信道间隔为1MHZ, 默认为02即2402MHz   */
    INT8U rf_dr;                            /* 数据传输速率，设置射频数据率为250kbps 、1Mbps 或2Mbps */
    INT8U rf_pwr;                           /* 设置TX发射功率111: 7dBm,  000:-12dBm */
} Rf_para_t;

/*
********************************************************************************
* 自运行任务
********************************************************************************
*/
typedef enum {
    TASK_TIME,                              /* 时控任务 */
    TASK_LONG_LAT,                          /* 经纬度任务 */
    TASK_LIGHT,                             /* 光照任务 */
    TASK_TYPE_MAX
} Task_type_e;

typedef union {
    INT8U type;
struct { 
        INT8U weekday:1;                    /* 周日 */
		INT8U monday:1;                     /* 周一 */
		INT8U thes:1;                       /* 周二 */
		INT8U wed:1;                        /* 周三 */
		INT8U thur:1;                       /* 周四 */
		INT8U fri:1;                        /* 周五 */
		INT8U sat:1;                        /* 周六 */
		INT8U dt:1;                         /* 模式 0:RTC模式, 1:待定 */
	} ch;
} Task_time_type_t;

typedef struct {
    Time_t time;                            /* 定时 */
    INT8U  ctl;                             /* 执行动作 */
    INT8U  dimming;                         /* 调光率 */
} Time_action_t;

#define ACTION_NUM_MAX	7                   /* 一次设置"定时/倒计时"最多的动作个数 */
#define TASK_NUM_MAX    7                   /* 支持"定时/倒计时"总任务个数 */
typedef struct {
    INT8U num;                              /* 动作总数 */
    Task_time_type_t type;                  /* 时间类型 */
    Time_action_t action[ACTION_NUM_MAX];
} Task_time_t;

typedef struct {
    double lon;                             /* 经度 */
    double lat;                             /* 纬度 */
} Long_lat_t;

typedef  struct 
{
    INT8U mode;                                    /* 任务当前工作模式 */
    Long_lat_t long_lat;                           /* 经纬度 */
    INT8U time_num[LAMP_NUM_MAX];                  /* 定时/倒计时 个数 */
	Task_time_t time[LAMP_NUM_MAX][TASK_NUM_MAX];  /* 定时/倒计时 任务 */
} Task_t;

    
/*
********************************************************************************
*                        Define Default PubParas
********************************************************************************
*/
#ifdef GLOBALS_PP_REG
const INT32U c_uid = 0xffffffff;

#if EN_ETHERNET > 0
const Ip_para_t c_ip_para = {
    {0xc8, 0x9c, 0xdc, 0x33, 0x91, 0xf9},
    {192, 168,  31, 120},
    {255, 255, 255,   0},
    {192, 168,  31,   1},
    {  0,   0,   0,   0},
       2,                                     /* 1 - Static, 2 - DHCP */ 
    {192, 168,  31, 130},
    8600
};
#endif

#if EN_WIFI > 0
const Wifi_para_t c_wifi_para = {
        "syj0925",
        "23258380",
        "api.yeelink.net",
        80
};
#endif

#if EN_NRF24L01 > 0
const Rf_para_t c_rf_para = {
        100,                      /* 分别对应1~125 个道,信道间隔为1MHZ, 默认为02即2402MHz */
        0x20,                     /* 250kbps:0x20, 1Mbps:0x00, 2Mbps:0x08; */
        0x07                      /* 0:-12dbm, 1:-6dbm, 2:-4dbm, 3:0dbm, 4:1dbm, 5:3dbm, 6:4dbm, 7:7dbm */
};
#endif

const Task_t c_task = {
    TASK_LONG_LAT,//TASK_TIME,
    {118.068377, 24.569995},
    {3},
    {
        {
            {2, {0x7f}, {{{0, 2, 0}, LAMP_OPEN, 50}, {{0, 5, 0}, LAMP_CLOSE, 0}}},
            {2, {0x7f}, {{{1, 0, 0}, LAMP_OPEN, 50}, {{1, 5, 0}, LAMP_CLOSE, 0}}},
            {2, {0x7f}, {{{2, 0, 0}, LAMP_OPEN, 50}, {{2, 5, 0}, LAMP_CLOSE, 0}}}
        }
    }
};


#endif
#endif

