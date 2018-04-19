/********************************************************************************
**
** 文件名:     alarm_v_leakage.c
** 版权所有:   (c) 2013-2015 
** 文件描述:   漏电压报警
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
#include "alarm_v_leakage.h"

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
#define UPPER_LIMIT     36         /* 漏电压触发报警上限阈值 */
#define LOWER_LIMIT     34         /* 漏电压解除报警下限阈值 */
#define COUNT_MAX       3

/*
********************************************************************************
* 结构定义
********************************************************************************
*/
typedef struct {
    INT8S status;           /* 报警状态 */
    INT8U count;
} Priv_t;

static INT8S _alarm_v_leakage_scan(Alarm_t* thiz)
{
	DECL_PRIV(thiz, priv);
	return_val_if_fail(priv != NULL, -1);

    if (thiz->id == V_LEAKAGE_POLE) {
        if ((priv->status != 1) && (g_power.v_leakage_eff_avr >= UPPER_LIMIT)) {
            if (priv->count++ > COUNT_MAX) {
                priv->count  = 0;
                priv->status = 1;
                SYS_DEBUG("<alarm_v_leakage trigger>\n");
            }
        } else if ((priv->status == 1) && (g_power.v_leakage_eff_avr <= LOWER_LIMIT)) {
            if (priv->count++ > COUNT_MAX) {
                priv->count  = 0;
                priv->status = 0;
                SYS_DEBUG("<alarm_v_leakage relieve>\n");
            }
        } else {
            priv->count = 0;
        }
    } else {
        priv->status = -1;                                                     /* -1代表未知状态 */
    }

    return priv->status;
}

static INT8S _alarm_v_leakage_check(Alarm_t* thiz)
{
	DECL_PRIV(thiz, priv);

	return priv->status;
}

static void _alarm_v_leakage_destroy(Alarm_t* thiz)
{
	if (thiz != NULL) {
		//DECL_PRIV(thiz, priv);
		mem_free(thiz);
	}

	return;
}

Alarm_t* alarm_v_leakage_create(Alarm_id_e id)
{
	Alarm_t* thiz = (Alarm_t*)mem_malloc(sizeof(Alarm_t) + sizeof(Priv_t));

	if (thiz != NULL) {
		DECL_PRIV(thiz, priv);
        thiz->id       = id;
		thiz->scan     = _alarm_v_leakage_scan;
		thiz->check    = _alarm_v_leakage_check;
		thiz->destroy  = _alarm_v_leakage_destroy;
        
        priv->status   = 0;
        priv->count    = 0;
	}

	return thiz;
}

