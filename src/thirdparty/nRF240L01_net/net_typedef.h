/********************************************************************************
**
** 文件名:     net_typedef.h
** 版权所有:   (c) 2013-2015 
** 文件描述:   common used type definition.
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**|    日期    |  作者  |  修改记录
**===============================================================================
**| 2015/07/04 | 苏友江 |  创建该文件
********************************************************************************/
#ifndef NET_TYPEDEF_H
#define NET_TYPEDEF_H

/*
********************************************************************************
* 宏定义
********************************************************************************
*/

#ifndef	 _SUCCESS
#define  _SUCCESS                0
#endif

#ifndef	 _FAILURE
#define  _FAILURE                1
#endif

#ifndef  _OVERTIME
#define  _OVERTIME               2
#endif

#define ENDDEVICE                0
#define ROUTER                   1
/*
********************************************************************************
* 参数配置
********************************************************************************
*/
#define DIRECT_SEND          0    /* 直接发送，终端节点不能进入休眠模式 */
#define PASSIVE_SEND         1    /* 被动发送，收到终端数据后，马上发送数据给终端 */

#define SEND_MODE            PASSIVE_SEND


/*
********************************************************************************
* 数据类型定义
********************************************************************************
*/
#define LINK_HEAD_LEN    3
#define PLOAD_WIDTH      32
#define ADR_WIDTH        1

typedef struct {
    INT8U destaddr[ADR_WIDTH];
    INT8U srcaddr[ADR_WIDTH];
    INT8U type;
    INT8U data[1];
} LINK_HEAD_T;


/*
********************************************************************************
* 参数配置
********************************************************************************
*/
#define NET_NODE_MAX     20
#define NODE_NAME_MAX    10
#define LIVE_TIME_MAX    5*60            /* 节点生存时间，单位S */

#define ONLINE           0x00
#define OFFLINE          0x01

/*
********************************************************************************
* 结构定义
********************************************************************************
*/
union bit_def
{
    INT8U b8;
    struct bit8_def
    {
        INT8U rain:1;
        INT8U motion:1;
        INT8U mq2:1;
        INT8U b3:1;
        INT8U b4:1;
        INT8U b5:1;
        INT8U b6:1;
        INT8U b7:1;
    } b;
};

typedef struct {
    INT16S temperature;
    
    union bit_def status;
} Sensor_t;

/* 统计信息 */
typedef struct {
    INT16U  log_cnt;                     /* 终端登陆次数 */

    
} Net_link_node_info_t;

/* 网络节点信息 */
typedef struct {
    INT8U  flowseq;                      /* 入网请求流水号，用来区分同一次入网请求的不同路由器转发的多包针 */
    INT8U  type;                         /* 节点类型，目前有路由器和终端节点两种 */
    INT8U  level;                        /* 网络级数-路由次数 */
    INT8U  status;                       /* 节点状态，目前就在线和掉线两种 */
    INT8U  addr[ADR_WIDTH];              /* 节点地址 */
    INT8U  routeaddr[ADR_WIDTH];         /* 本节点的路由地址 */
    INT8U  name[NODE_NAME_MAX];          /* 节点名称 */
    INT16U live;                         /* 存活时间单位：second，在存活时间内无更新，将删除该节点，挂到离线链表上 */
    Net_link_node_info_t node_info;
    Sensor_t sensor;                     /* 传感器信息 */
} Net_link_node_t;

typedef struct {
    Net_link_node_t *node;
    INT8U len;
    INT8U *pdata;
    INT8U buf[PLOAD_WIDTH];
} PACKET_BUF_T;






#endif 


