/********************************************************************************
**
** 文件名:     energy_measure.c
** 版权所有:   (c) 2013-2015 
** 文件描述:   电能测量模块
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**|    日期    |  作者  |  修改记录
**===============================================================================
**| 2015/08/31 | 苏友江 |  创建该文件
********************************************************************************/
#include <math.h>
#include "bsp.h"
#include "sys_includes.h"

#if DBG_ENERGY_MEASURE > 0
#define SYS_DEBUG          OS_DEBUG
#else
#define SYS_DEBUG(...)     do{}while(0)
#endif
/*
********************************************************************************
* 定义内部私有结构体
********************************************************************************
*/
typedef struct {
    INT8U cnt;
    float sum_v;
    float sum_v_leakage;
    float sum_i[LAMP_NUM_MAX];
    float sum_p[LAMP_NUM_MAX];
} Priv_t;


/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/
Power_t g_power = {0};
static Priv_t  s_priv = {0};
static INT8U   s_measuretmr;
static INT16S  s_dc_avr[AD_CH_MAX] = {2048, 2048, 2048};

/*******************************************************************
** 函数名:      _get_ad_channel
** 函数描述:    根据灯索引号获取指定ad通道
** 参数:        [in] lamp_index: 灯索引
** 返回:        ad通道
********************************************************************/
static INT8U _get_ad_channel(INT8U lamp_index)
{
	INT8U ad_ch = 0;

    switch (lamp_index)
    {
        case LAMP1_INDEX:
            ad_ch = AD_CH_LAMP1_I;
            break;
        default:
            break;
    }

    return ad_ch;
}

/*******************************************************************
** 函数名:      remove_dc
** 函数描述:    去除AD采样中的直流分量
** 参数:        none
** 返回:        none
********************************************************************/
static void _remove_dc(void)
{
	INT8U i, j;
	INT32S sum[AD_CH_MAX] = {0};
	static INT32S dc_avr_sum[AD_CH_MAX] = {0};
	static INT8U dc_avr_cnt = 0;
	#ifdef MEASURE_AC
	INT8U up_down_dc_avr_cnt[AD_CH_MAX] = {0};
	INT8U abs_dc_avr = 0;
	#endif
    
	for (i = 0; i < AD_CH_MAX; i++) {
		for (j = 1; j < SAMPLE+1; j++) {
			sum[i] += g_ad_cvs.buf[i][j];
		}
		dc_avr_sum[i] += ((sum[i]*10)/SAMPLE);
	}
	dc_avr_cnt++;
	
	if (dc_avr_cnt >= AVR_DIVISOR) {
		dc_avr_cnt = 0;  
		for (i = 0; i < AD_CH_MAX; i++) {
			s_dc_avr[i] = (INT16S)((dc_avr_sum[i]+5)/10/AVR_DIVISOR);
			dc_avr_sum[i] = 0;
		}
	}
    
	#ifdef MEASURE_AC	
	for (i = 0; i < AD_CH_MAX; i++) {
		for (j = 1; j < SAMPLE+1; j++) {
			g_ad_cvs.buf[i][j] -= s_dc_avr[i];
			abs_dc_avr = ABS(g_ad_cvs.buf[i][j]);
			if (abs_dc_avr <= 5) {                                             /* ad平均值上下，小于5 */
				up_down_dc_avr_cnt[i]++;
			}	
		}
	}
	
	for (i = 0; i < AD_CH_MAX; i++) {	
		if (up_down_dc_avr_cnt[i] >= SAMPLE-4) {                               /* 表示空载时 */
			up_down_dc_avr_cnt[i] = 0;

			for (j = 1; j < SAMPLE+1; j++) {
				if (g_ad_cvs.buf[i][j] > 0) {                                  /* 修正幅值和对称性 */
					if ((j%2) == 0)
						g_ad_cvs.buf[i][j] = 1;
					else
						g_ad_cvs.buf[i][j] = -1;
				} else if (g_ad_cvs.buf[i][j] < 0) {
					if ((j%2) == 0)
						g_ad_cvs.buf[i][j] = 1;
					else
						g_ad_cvs.buf[i][j] = -1;
				}
			}
		}
	}
	#endif 
}

/*******************************************************************
** 函数名:      _calculate_ac_fr
** 函数描述:    计算交流电的频率
** 参数:        none
** 返回:        频率
********************************************************************/
static INT8U _calculate_ac_fr(void)
{
	float period, f;
	INT8U j, cnt = 0, start = 0, f_temp = 0;
    static INT8U f_value = 50, f_value_old = 50, f_cnt = 0, f_avr = 50;
    static INT16U f_sum = 0;

	for (j = 1; j < SAMPLE; j++) {
		if (g_ad_cvs.buf[AD_CH_LAMPS_V][j] == g_ad_cvs.buf[AD_CH_LAMPS_V][j+1]) {
			return f_value;
        }
        
		if (start == 1) {
			cnt++;
			if ((g_ad_cvs.buf[AD_CH_LAMPS_V][j] < 0) && (g_ad_cvs.buf[AD_CH_LAMPS_V][j+1] > 0)) { 	
				start = 2;
				break;
			}
		}
        
		if ((g_ad_cvs.buf[AD_CH_LAMPS_V][j] > 0) && (g_ad_cvs.buf[AD_CH_LAMPS_V][j+1] < 0) && (start == 0)) {
			start = 1;
		}

		if (start == 3) {
			cnt++;
			if ((g_ad_cvs.buf[AD_CH_LAMPS_V][j] > 0) && (g_ad_cvs.buf[AD_CH_LAMPS_V][j+1] < 0))	{
				start = 4;
				break;
			}
		}
        
		if ((g_ad_cvs.buf[AD_CH_LAMPS_V][j] < 0) && (g_ad_cvs.buf[AD_CH_LAMPS_V][j+1] > 0) && (start == 0)) {
			start = 3;
		}
	}

	period = cnt*2*(1000.0/f_value/SAMPLE);
	f = 1000/period;
	
	if (f_cnt < 8) {
		f_temp = (INT8U)f;
		if ((f_temp >= 70) || (f_temp <= 40)) {
			return f_value;
        }
		f_sum += f_temp;
		f_cnt++;
		return f_value;
	} else {
		f_cnt = 0;
		f_avr = f_sum>>3;
		f_sum = 0;
	}
    
	if ((f_avr > 40) && (f_avr <= 55)) {
		f_value = 50;
		if (f_value_old != f_value) {
			SYS_DEBUG("renew frequence to 50hz\r\n");
			SYS_DEBUG("lamp all wait 1 min then chk err\r\n");				
		}
		f_value_old = f_value;
	} else if ((f_avr > 55) && (f_avr < 70)) {
		f_value = 60;
		if (f_value_old != f_value) {
			SYS_DEBUG("renew frequence to 60hz\r\n");
			SYS_DEBUG("lamp all wait 1 min then chk err\r\n");				
		}
		f_value_old = f_value;
	}
	return f_value;
}


/*******************************************************************
** 函数名:      _calculate_ac_v
** 函数描述:    计算交流电压值
** 参数:        none
** 返回:        电压
********************************************************************/
static float _calculate_ac_v(INT8U ad_ch)
{
	INT8U i;
	float sum = 0, buf_temp;

	for (i = 1; i < SAMPLE+1; i++) {
		buf_temp = INTO_VOLTAGE(g_ad_cvs.buf[ad_ch][i]);
		sum += SQUARE(buf_temp);
	}
	
	return(SQRT(sum/SAMPLE)); 
}

/*******************************************************************
** 函数名:      _calculate_ac_i
** 函数描述:    计算交流电流值
** 参数:        none
** 返回:        电流
********************************************************************/
static float _calculate_ac_i(INT8U lamp_index)
{
	INT8U i, ad_ch = 0;
	float sum = 0, buf_temp;

    ad_ch = _get_ad_channel(lamp_index);
	for (i = 1; i < SAMPLE+1; i++) {
		buf_temp = INTO_CURRENT(g_ad_cvs.buf[ad_ch][i]);
		sum += SQUARE(buf_temp);
	}
    
	return(SQRT(sum/SAMPLE));
}

/*******************************************************************
** 函数名:      _calculate_ac_p
** 函数描述:    计算交流有功功率
** 参数:        none
** 返回:        有功功率
********************************************************************/
static float _calculate_ac_p(INT8U lamp_index)
{
	float sum = 0;
	INT8U i, ad_ch = 0;

    ad_ch = _get_ad_channel(lamp_index);
	for (i = 1; i < SAMPLE+1; i++) {
		sum += INTO_VOLTAGE(g_ad_cvs.buf[AD_CH_LAMPS_V][i])*INTO_CURRENT(g_ad_cvs.buf[ad_ch][i]);
	}
    
	return ABS(sum/SAMPLE);
}

/*******************************************************************
** 函数名:      _power_calculate
** 函数描述:    相关电能计算
** 参数:        none
** 返回:        none
********************************************************************/
static void _power_calculate(void)
{
    INT8U i;
    float v_temp;
    static INT8U cnt = 0;
    static BOOLEAN issave = 0;

    _remove_dc();
    v_temp = _calculate_ac_v(AD_CH_LAMPS_V);
    s_priv.sum_v += v_temp;
    s_priv.sum_v_leakage += _calculate_ac_v(AD_CH_LEAKA_V);
    
	for (i = 0; i < LAMP_NUM_MAX; i++) {
        s_priv.sum_i[i] += _calculate_ac_i(i);
        s_priv.sum_p[i] += _calculate_ac_p(i);
	}

    if (++s_priv.cnt >= AVR_DIVISOR) {
    	g_power.fr                = _calculate_ac_fr();	
		g_power.v_eff_avr         = s_priv.sum_v/AVR_DIVISOR;
		g_power.v_leakage_eff_avr = s_priv.sum_v_leakage/AVR_DIVISOR;

        g_power.p_all_lamps = 0;
    	for (i = 0; i < LAMP_NUM_MAX; i++) {
            g_power.lamp[i].i_eff_avr = s_priv.sum_i[i]/AVR_DIVISOR;
            g_power.lamp[i].p_avr     = s_priv.sum_p[i]/AVR_DIVISOR;

            g_power.lamp[i].s_avr = g_power.v_eff_avr*g_power.lamp[i].i_eff_avr;
            
        	if (g_power.lamp[i].s_avr > 0) {
                g_power.lamp[i].pf = g_power.lamp[i].p_avr/g_power.lamp[i].s_avr;
        		if (g_power.lamp[i].pf > 1) {
                    g_power.lamp[i].pf = 1;
                }
        	} else {
        		g_power.lamp[i].pf = 0;
            }
    		g_power.p_all_lamps += g_power.lamp[i].p_avr;
    	}
        
        memset(&s_priv, 0, sizeof(Priv_t));
    }

    /* 断电检测，检测到电压联系几次小于80v，则认为断电，通知系统存储重要数据 */
    if ((g_run_sec > 10) && (v_temp <= 80)) {
        if (issave == 0 && cnt++ > 3) {
            issave = 1;
            energy_measure_power_store();
            SYS_DEBUG("<power off:%f>\n", v_temp);
        }
    } else {
        if ((issave == 1) && (v_temp >= 200)) {
            issave = 0;
        }
        cnt = 0;
    }
}

/*******************************************************************
** 函数名:     _energy_measure_tmr
** 函数描述:   energy measure定时器
** 参数:       [in] index  : 定时器参数
** 返回:       无
********************************************************************/
static void _energy_measure_tmr(void *index)
{   
    INT8U i;
    static BOOLEAN is_collect = 0;
    static INT32U last_time = 0;                                               /* 上电10s后，开始统计，因为刚上电计算的电压、电流等误差很大 */
    index = index;

    if (is_collect == 0) {
     	g_ad_cvs.sample = 0;
    	adc_timer_start();
        is_collect = 1;
		os_timer_start(s_measuretmr, 1);
    }
	
	if (g_ad_cvs.sample < SAMPLE_NUM) {                                        /* 检测是否采集完指定样品 */
        return;
	} else {
        is_collect = 0;
    }
	_power_calculate();

    if (g_run_sec > last_time) {
        INT8U interval = g_run_sec - last_time;
        last_time = g_run_sec;
        
        for (i = 0; i < LAMP_NUM_MAX; i++) {
            g_power.lamp[i].ws += g_power.lamp[i].p_avr*interval;
            
            if (lamp_is_open((Lamp_e)i)) {
                g_power.lamp[i].t_light += interval;
            }
        }

        if (g_run_sec%10 == 0) {
            SYS_DEBUG("<fr:%d, v:%f, i:%f, p:%f, s:%f, pf:%f, ws:%f, t:%d>\n", g_power.fr, g_power.v_eff_avr,
                g_power.lamp[0].i_eff_avr, g_power.lamp[0].p_avr, g_power.lamp[0].s_avr,
                g_power.lamp[0].pf, g_power.lamp[0].ws, g_power.lamp[0].t_light);
        }
        
        /* 1小时存储一次电能数据 */
        if (g_run_sec%3600 < interval) {
            energy_measure_power_store();
        }
    }    
}

/*******************************************************************
** 函数名:      energy_measure_init
** 函数描述:    电能测量模块初始化
** 参数:        无
** 返回:        无
********************************************************************/
void energy_measure_init(void)
{
    INT8U i;
    Power_total_t power_total;

    memset(&s_priv,  0, sizeof(Priv_t));
    memset(&g_power, 0, sizeof(Power_t));

    if (public_para_manager_check_valid_by_id(POWER_TOTAL_)) {
        public_para_manager_read_by_id(POWER_TOTAL_, (INT8U*)&power_total, sizeof(Power_total_t));
        for (i = 0; i < LAMP_NUM_MAX; i++) {
            g_power.lamp[i].ws = power_total.lamp_ws[i];
            g_power.lamp[i].t_light = power_total.t_light[i];
        }
    }
    
    s_measuretmr = os_timer_create(0, _energy_measure_tmr);
    os_timer_start(s_measuretmr, 1);
}

/*******************************************************************
** 函数名:      energy_measure_power_store
** 函数描述:    存储电能数据到eeprom中
** 参数:        none
** 返回:        none
********************************************************************/
void energy_measure_power_store(void)
{
    INT8U i;
    Power_total_t power_total;

    for (i = 0; i < LAMP_NUM_MAX; i++) {
        power_total.lamp_ws[i] = g_power.lamp[i].ws;
        power_total.t_light[i] = g_power.lamp[i].t_light;
    }

    public_para_manager_store_instant_by_id(POWER_TOTAL_, (INT8U*)&power_total, sizeof(Power_total_t));
}

