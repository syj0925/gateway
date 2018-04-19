/********************************************************************************
**
** 文件名:     alarm.h
** 版权所有:   (c) 2013-2015 
** 文件描述:   报警抽象
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**|    日期    |  作者  |  修改记录
**===============================================================================
**| 2015/09/24 | 苏友江 |  创建该文件
********************************************************************************/
#ifndef ALARM_H
#define ALARM_H



typedef enum {
    V_LEAKAGE_POLE,               /* 灯杆漏电压 */
    LAMP1_TURN_ON_FAULT,          /* 灯1开灯故障 */
    LAMP1_TURN_OFF_FAULT,         /* 灯1关灯故障 */
    ALARM_ID_MAX
} Alarm_id_e;



struct _Alarm;
typedef struct _Alarm Alarm_t;

typedef INT8S  (*AlarmScan)(Alarm_t* thiz);
typedef INT8S  (*AlarmCheck)(Alarm_t* thiz);
typedef void (*AlarmDestroy)(Alarm_t* thiz);

struct _Alarm {
	AlarmScan     scan;
	AlarmCheck    check;
	AlarmDestroy  destroy;
    Alarm_id_e    id;
	char priv[1];
};

static __inline INT8S alarm_scan(Alarm_t* thiz)
{
	return_val_if_fail(thiz != NULL && thiz->scan != NULL, -1);

	return thiz->scan(thiz);
}

static __inline INT8S alarm_check(Alarm_t* thiz)
{
	return_val_if_fail(thiz != NULL && thiz->check != NULL, -1);

	return thiz->check(thiz);
}

static __inline void alarm_destroy(Alarm_t* thiz)
{
	if(thiz != NULL && thiz->destroy != NULL)
	{
		thiz->destroy(thiz);
	}

	return;
}

#endif

