/********************************************************************************
**
** 文件名:     para_manager.c
** 版权所有:   (c) 2013-2015 
** 文件描述:   参数管理模块
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**|    日期    |  作者  |  修改记录
**===============================================================================
**| 2015/09/16 | 苏友江 |  创建该文件
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
extern Sys_base_info_t g_sys_base_info;
/*******************************************************************
** 函数名:      _handle_0x8206
** 函数描述:    参数设置
** 参数:        [in] packet:
** 返回:        none
********************************************************************/
static void _handle_0x8206(Comm_pkt_recv_t *packet)
{
    Stream_t rstrm;
    INT8U i, j, temp, para_num, para_id, para_len, result = 0;
    BOOLEAN task = 0;
    Systime_t time;

    stream_init(&rstrm, packet->pdata, packet->len);
    para_num = stream_read_byte(&rstrm);

    for (i = 0; i < para_num; i++) {
        para_id  = stream_read_byte(&rstrm);
        para_len = stream_read_byte(&rstrm);

        switch(para_id) {
            case 0x01:
                if (para_len == 7) {  
                    time.tm_year = stream_read_byte(&rstrm)+100;
                    time.tm_mon  = stream_read_byte(&rstrm);
                    time.tm_mday = stream_read_byte(&rstrm);
                    time.tm_wday = stream_read_byte(&rstrm);
                    time.tm_hour = stream_read_byte(&rstrm);
                    time.tm_min  = stream_read_byte(&rstrm);
                    time.tm_sec  = stream_read_byte(&rstrm);
                    os_systime_set(&time);
                } else {
                    stream_move_pointer(&rstrm, para_len);
                    result = 1;
                }
                break;
            case 0x02:
                temp = stream_read_byte(&rstrm);
                if ((temp <= LAMP_NUM_MAX) && (para_len == (1+temp*8))) {
                    for (j = 0; j < temp; j++) {
                        *((INT32U*)&g_power.lamp[i].ws) = stream_read_long(&rstrm);
                        g_power.lamp[i].t_light = stream_read_long(&rstrm);
                    }
                    energy_measure_power_store();
                } else {
                    stream_move_pointer(&rstrm, para_len - 1);
                    result = 1;
                }
                break;
            case 0x03:
                task = 1;
                g_task_para.mode = stream_read_byte(&rstrm);
                break;
            case 0x04:
                if (para_len == 2*sizeof(double)) {
                    task = 1;
                    stream_read_data_back(&rstrm, (INT8U*)&g_task_para.long_lat.lon, sizeof(g_task_para.long_lat.lon));
                    stream_read_data_back(&rstrm, (INT8U*)&g_task_para.long_lat.lat, sizeof(g_task_para.long_lat.lat));
                } else {
                    stream_move_pointer(&rstrm, para_len);
                    result = 1;
                }
                break;
            case 0x05:
            {
                INT8U k, l, lamp_num, lamp_index;
                INT16U read_len;

                read_len = stream_get_len(&rstrm);
                if ((lamp_num = stream_read_byte(&rstrm)) > LAMP_NUM_MAX) {
                    goto TIME_TASK_ERR;
                }

                for (j = 0; j < lamp_num; j++) {
                    lamp_index = stream_read_byte(&rstrm);
                    if (lamp_index >= LAMP_NUM_MAX) {
                        goto TIME_TASK_ERR;
                    }
                    
                    g_task_para.time_num[lamp_index] = stream_read_byte(&rstrm);
                    if (g_task_para.time_num[lamp_index] > TASK_NUM_MAX) {
                        goto TIME_TASK_ERR;
                    }
                    
                    for (k = 0; k < g_task_para.time_num[lamp_index]; k++) {
                        g_task_para.time[lamp_index][k].num = stream_read_byte(&rstrm);
                        if (g_task_para.time[lamp_index][k].num > ACTION_NUM_MAX) {
                            goto TIME_TASK_ERR;
                        }
                        g_task_para.time[lamp_index][k].type.type = stream_read_byte(&rstrm);
                        for (l = 0; l < g_task_para.time[lamp_index][k].num; l++) {
                            g_task_para.time[lamp_index][k].action[l].time.hour   = stream_read_byte(&rstrm);
                            g_task_para.time[lamp_index][k].action[l].time.minute = stream_read_byte(&rstrm);
                            g_task_para.time[lamp_index][k].action[l].time.second = stream_read_byte(&rstrm);

                            g_task_para.time[lamp_index][k].action[l].ctl     = stream_read_byte(&rstrm);
                            g_task_para.time[lamp_index][k].action[l].dimming = stream_read_byte(&rstrm);
                        }
                    }
                }
                
                if (para_len != stream_get_len(&rstrm) - read_len) {
                    goto TIME_TASK_ERR;
                }
                task = 1;
                break;
TIME_TASK_ERR:                
                result = 1;
                public_para_manager_read_by_id(TASK_, (INT8U*)&g_task_para, sizeof(Task_t));
                goto ERROR;
            }
            
            case 0x06:
                g_sys_base_info.uid = stream_read_long(&rstrm);
                public_para_manager_store_instant_by_id(UID_, (INT8U*)&g_sys_base_info.uid, sizeof(g_sys_base_info.uid));
                result = 1;
                break;

            case 0x07:
            {                
                stream_read_data(&rstrm, g_sys_base_info.group, GROUP_NUM);
                public_para_manager_store_instant_by_id(GROUP_, g_sys_base_info.group, GROUP_NUM);
                result = 1;
                break;
            }
            default:
                stream_move_pointer(&rstrm, para_len);
                result = 1;
                break;
        }
    }

    if (task) {
        public_para_manager_store_instant_by_id(TASK_, (INT8U*)&g_task_para, sizeof(Task_t));
    }

ERROR:
    if (packet->ack) {
        comm_protocol_common_ack(packet->flowseq, 0x8206, result);
    }
}

/*******************************************************************
** 函数名:      _handle_0x8207
** 函数描述:    参数查询
** 参数:        [in] packet:
** 返回:        none
********************************************************************/
static void _handle_0x8207(Comm_pkt_recv_t *packet)
{
    #define SENDBUF_SIZE    50
    Stream_t rstrm, wstrm;
    INT8U i, j, para_num, para_id, *psendbuf;
    Comm_pkt_send_t send_pkt;

    if (!packet->ack) {/* 无需应答 */
        return;
    }

    psendbuf = (INT8U*)mem_malloc(SENDBUF_SIZE);
    if (psendbuf == NULL) {
        return;
    }
    stream_init(&wstrm, psendbuf, SENDBUF_SIZE);
    stream_write_half_word(&wstrm, packet->flowseq);

    stream_init(&rstrm, packet->pdata, packet->len);
    para_num = stream_read_byte(&rstrm);
    stream_write_byte(&wstrm, para_num);

    for (i = 0; i < para_num; i++) {
        para_id  = stream_read_byte(&rstrm);
        stream_write_byte(&wstrm, para_id);
        switch(para_id) {
            case 0x01:
                stream_write_byte(&wstrm, 7);
                stream_write_byte(&wstrm, g_systime.tm_year-100);
                stream_write_byte(&wstrm, g_systime.tm_mon);
                stream_write_byte(&wstrm, g_systime.tm_mday);
                stream_write_byte(&wstrm, g_systime.tm_wday);
                stream_write_byte(&wstrm, g_systime.tm_hour);
                stream_write_byte(&wstrm, g_systime.tm_min);
                stream_write_byte(&wstrm, g_systime.tm_sec);
                break;
            case 0x02:
                stream_write_byte(&wstrm, 1+LAMP_NUM_MAX*8);
                stream_write_byte(&wstrm, LAMP_NUM_MAX);
                for (j = 0; j < LAMP_NUM_MAX; j++) {
                    stream_write_long(&wstrm, *((INT32U*)&g_power.lamp[i].ws));
                    stream_write_long(&wstrm, g_power.lamp[i].t_light);
                }
                break;
            case 0x03:
                stream_write_byte(&wstrm, 1);
                stream_write_byte(&wstrm, g_task_para.mode);
                break;
            case 0x04:
                stream_write_byte(&wstrm, 2*sizeof(double));
                stream_write_data_back(&wstrm, (INT8U*)&g_task_para.long_lat.lon, sizeof(g_task_para.long_lat.lon));
                stream_write_data_back(&wstrm, (INT8U*)&g_task_para.long_lat.lat, sizeof(g_task_para.long_lat.lat));
                break;
            case 0x05:
            {
                INT8U *plen, k, l;

                plen = stream_get_pointer(&wstrm);
                stream_move_pointer(&wstrm, 1);
                
                stream_write_byte(&wstrm, LAMP_NUM_MAX);
                for (j = 0; j < LAMP_NUM_MAX; j++) {
                    stream_write_byte(&wstrm, j);
                    stream_write_byte(&wstrm, g_task_para.time_num[j]);
                    for (k = 0; k < g_task_para.time_num[j]; k++) {
                        stream_write_byte(&wstrm, g_task_para.time[j][k].num);
                        stream_write_byte(&wstrm, g_task_para.time[j][k].type.type);

                        for (l = 0; l < g_task_para.time[j][k].num; l++) {
                            stream_write_byte(&wstrm, g_task_para.time[j][k].action[l].time.hour);
                            stream_write_byte(&wstrm, g_task_para.time[j][k].action[l].time.minute);
                            stream_write_byte(&wstrm, g_task_para.time[j][k].action[l].time.second);
                            stream_write_byte(&wstrm, g_task_para.time[j][k].action[l].ctl);
                            stream_write_byte(&wstrm, g_task_para.time[j][k].action[l].dimming);
                        }
                    }
                }

                *plen = (stream_get_pointer(&wstrm) - plen) - 1;
                break;
            } 

            case 0x07:
            {                
                stream_write_data(&wstrm, g_sys_base_info.group, GROUP_NUM);
                break;
            }
            default:
                stream_write_byte(&wstrm, 0);                                  /* 未知参数，长度设置为0，代表暂不存在该参数 */
                break;
        }
    }

    send_pkt.len   = stream_get_len(&wstrm);
    send_pkt.pdata = psendbuf;
    send_pkt.msgid = 0x0207;
    comm_send_dirsend(&send_pkt);
    mem_free(psendbuf);
}

/*
********************************************************************************
* 注册回调函数
********************************************************************************
*/
static const FUNCENTRY_COMM_T s_functionentry[] = {
        0x8206, _handle_0x8206       /* 终端参数设置 */
       ,0x8207, _handle_0x8207       /* 终端参数查询 */
};
static const INT8U s_funnum = sizeof(s_functionentry) / sizeof(s_functionentry[0]);

/*******************************************************************
** 函数名:      para_manager_init
** 函数描述:    参数管理模块初始化
** 参数:        无
** 返回:        无
********************************************************************/
void para_manager_init(void)
{
	INT8U i;

    for (i = 0; i < s_funnum; i++) {
        comm_recv_register(s_functionentry[i].index, s_functionentry[i].entryproc);
    }
}

