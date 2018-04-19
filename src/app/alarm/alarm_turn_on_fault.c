/********************************************************************************
**
** 文件名:     alarm_turn_on_fault.c
** 版权所有:   (c) 2013-2015 
** 文件描述:   开灯故障报警
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**|    日期    |  作者  |  修改记录
**===============================================================================
**| 2015/09/26 | 苏友江 |  创建该文件
********************************************************************************/
#include "bsp.h"
#include "sys_includes.h"
#include "alarm.h"
#include "alarm_turn_on_fault.h"

#if DBG_ALARM > 0
#define SYS_DEBUG          OS_DEBUG
#else
#define SYS_DEBUG(...)     do{}while(0)
#endif
/*
********************************************************************************
* 参数配置
********************************************************************************
*/
#define COUNT_MAX       3

/*
********************************************************************************
* 结构定义
********************************************************************************
*/
typedef struct {
	INT16U pf_lower_limit;
	INT16U p_higher_limit;  
	INT16U p_lower_limit;  
	INT16U i_higher_limit;
	INT16U i_lower_limit;
} Para_t;

typedef struct {
    Lamp_e lamp;                   /* 灯具索引号 */
    INT8S  status;                 /* 报警状态 */
    INT8U  count;
} Priv_t;


static const Para_t c_fault_para[1] = {
    {1000, 0,   40,    0,   400  }   /* 判断开灯故障参数阈值 */
};

static INT8S _alarm_turn_on_scan(Alarm_t* thiz)
{
	DECL_PRIV(thiz, priv);
	return_val_if_fail(priv != NULL, -1);

    if (priv->lamp < LAMP_NUM_MAX) {
        INT8U index;
    	INT16U lamp_pf = (INT16U)(g_power.lamp[priv->lamp].pf*1000);
    	INT16U lamp_i  = (INT16U)(g_power.lamp[priv->lamp].i_eff_avr*1000);
    	INT16U lamp_p  = (INT16U)(g_power.lamp[priv->lamp].p_avr*10);
        
        if (!lamp_is_open(priv->lamp) || (lamp_get_dimming(priv->lamp) < 30)) {/* 关灯状态，就不用判断开灯故障，返回原来的报警状态 */
            return priv->status;
        }
        
        index = 0;
		/* 要<=或>=，不然如果是<或>，出现0<0，0>0不成立 */
        if ((lamp_pf <= c_fault_para[index].pf_lower_limit)		
           &&(lamp_i <= c_fault_para[index].i_lower_limit)
           &&(lamp_i >= c_fault_para[index].i_higher_limit)			
           &&(lamp_p <= c_fault_para[index].p_lower_limit)
           &&(lamp_p >= c_fault_para[index].p_higher_limit))
        {	
            if (priv->status != 1) {
                if (priv->count++ > COUNT_MAX) {
                    priv->count  = 0;
                    priv->status = 1;
                    SYS_DEBUG("<alarm_turn_on trigger>\n");
                }
            }
        } else {
            if (priv->status == 1) {
                if (priv->count++ > COUNT_MAX) {
                    priv->count  = 0;
                    priv->status = 0;
                    SYS_DEBUG("<alarm_turn_on relieve>\n");
                }
            }
        }
    } else {
        priv->status = -1;                                                     /* -1代表未知状态 */
    }

    return priv->status;
}

static INT8S _alarm_turn_on_check(Alarm_t* thiz)
{
	DECL_PRIV(thiz, priv);

	return priv->status;
}

static void _alarm_turn_on_destroy(Alarm_t* thiz)
{
	if (thiz != NULL) {
		//DECL_PRIV(thiz, priv);
		mem_free(thiz);
	}

	return;
}

Alarm_t* alarm_turn_on_create(Alarm_id_e id)
{
	Alarm_t* thiz = (Alarm_t*)mem_malloc(sizeof(Alarm_t) + sizeof(Priv_t));

	if (thiz != NULL) {
		DECL_PRIV(thiz, priv);
        thiz->id       = id;
		thiz->scan     = _alarm_turn_on_scan;
		thiz->check    = _alarm_turn_on_check;
		thiz->destroy  = _alarm_turn_on_destroy;

        if (id == LAMP1_TURN_ON_FAULT) {
            priv->lamp = LAMP1_INDEX;
        } else {
            priv->lamp = LAMP_NUM_MAX;
        }
        priv->status   = 0;
        priv->count    = 0;
	}

	return thiz;
}

