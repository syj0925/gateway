/********************************************************************************
**
** 文件名:     sys_debugcfg.h
** 版权所有:   (c) 2013-2014 
** 文件描述:   debug系统配置
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**|    日期    |  作者  |  修改记录
**===============================================================================
**| 2016/01/15 | 苏友江 |  创建该文件
********************************************************************************/
#ifndef SYS_DEBUGCFG_H
#define SYS_DEBUGCFG_H        1


/**********************************************************
**              定义调试参数
**********************************************************/
#define EN_DEBUG                    1              /* 总编译调试开关 */

#define DEBUG_UART_NO               COM1           /* COM1~COM3 */
#define DEBUG_UART_BAUD             115200         /* 波特率 */


/*
*********************************************************
**              定义APP层调试开关
*********************************************************
*/
#if EN_DEBUG > 0

#define DBG_SYSTIME               0        /* 系统时间 */
#define DBG_PBULIC_PARA           1        /* 公共参数 */
#define DBG_SYSTEM                1        /* 系统杂项 */
#define DBG_ENERGY_MEASURE        0        /* 电能统计 */
#define DBG_ZIGBEE                0        /* ZIGBEE */
#define DBG_WIFI                  1        /* WIFI */
#define DBG_ETHERNET              0        /* 以太网 */
#define DBG_COMM                  1        /* 通信相关 */
#define DBG_GENERAL               0        /* 公共应用 */
#define DBG_ALARM                 0        /* 报警 */
#define DBG_YEELINK               0        /* yeelink */
#define DBG_DALI                  0        /* DALI调光 */
#define DBG_NRF24L01              1        /* NRF24L01网络 */

#else

#define DBG_SYSTIME               0        /* 系统时间 */
#define DBG_PBULIC_PARA           0        /* 公共参数 */
#define DBG_SYSTEM                0        /* 系统杂项 */
#define DBG_ENERGY_MEASURE        0        /* 电能统计 */
#define DBG_ZIGBEE                0        /* ZIGBEE */
#define DBG_WIFI                  0        /* WIFI */
#define DBG_ETHERNET              0        /* 以太网 */
#define DBG_COMM                  0        /* 通信相关 */
#define DBG_GENERAL               0        /* 公共应用 */
#define DBG_ALARM                 0        /* 报警 */
#define DBG_YEELINK               0        /* yeelink */
#define DBG_DALI                  0        /* DALI调光 */
#define DBG_NRF24L01              0        /* NRF24L01网络 */

#endif

extern BOOLEAN g_debug_sw;                                                     /* 用于动态开启关闭printf */

extern int OS_DEBUG(const char *format,...);
//#define OS_DEBUG     printf

#endif

