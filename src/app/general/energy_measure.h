/********************************************************************************
**
** 文件名:     energy_measure.h
** 版权所有:   (c) 2013-2015 
** 文件描述:   电能计算
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**|    日期    |  作者  |  修改记录
**===============================================================================
**| 2015/08/31 | 苏友江 |  创建该文件
********************************************************************************/
#ifndef ENERGY_MEASURE_H
#define ENERGY_MEASURE_H



#define   AVR_DIVISOR  50
#define   SAMPLE       32
//------电能测量AC or DC----------
#define   MEASURE_AC
//#define  MEASURE_DC

#define V_LEAKAGE

//-------参考电压5V or 4.096-------
#define REFERENCE_3V3

//----------校准系数--------------
#define V_CK        	1.002		   //电压校准系数
//#define  I_CK[4]        0.9990
//----------
#ifdef REFERENCE_5V
#define  INTO_CURRENT(x) (0.005086*(x))           /* (*5/4096)*1000/20/12---rtu09 */
#define  INTO_VOLTAGE(x) (0.366945*(x))	          /* (*5/4096)*150000/10/49.9*(x)---rtu09  */
#endif
#ifdef REFERENCE_4_096V
#define  INTO_CURRENT(x) (0.004167*(x))           /* (*4.096/4096)*1000/20/12---rtu06c */
#define  INTO_VOLTAGE(x) (0.300601*(x))	          /* (*4.096/4096)*150000/10/49.9*(x)---RTU06C */
#endif
#ifdef REFERENCE_3V3
#define  INTO_CURRENT(x) (0.003357*(x))           /* (*3.3/4096)*1000/20/12---rtu09 */
#define  INTO_VOLTAGE(x) (0.242183*(x))	          /* (*3.3/4096)*150000/10/49.9*(x)---rtu09  */
#endif

#ifdef MEASURE_DC
#define  INTO_DC_CURRENT(x)   (0.001299*(x))      /* (*3.3/4096)/31/0.02(x) */
#define  INTO_DC_VOLTAGE(x)   (0.010474*(x))	  /* (*3.3/4096)*13(x) */
#endif

typedef struct {
	float   i_eff_avr;		           /* 电流有效值 */
	float   p_avr;			           /* 有功功率 */
	//float q_avr;			           /* 无功功率 */
	float   s_avr;			           /* 视在功率 */
	float   pf;		                   /* 功率因素 */
	float   ws;		                   /* 瓦秒 */
    INT32U  t_light;                   /* 亮灯时长 */
} Lamp_power_t;

typedef struct {
	float   v_eff_avr;		           /* 电压有效值 */
	float   i_leakage_eff_avr;
	float   v_leakage_eff_avr;
	float   p_all_lamps;
	INT8U   fr;
    Lamp_power_t lamp[LAMP_NUM_MAX];
} Power_t;




extern Power_t g_power;

/*******************************************************************
** 函数名:      energy_measure_init
** 函数描述:    电能测量模块初始化
** 参数:        无
** 返回:        无
********************************************************************/
void energy_measure_init(void);

/*******************************************************************
** 函数名:      energy_measure_power_store
** 函数描述:    存储电能数据到eeprom中
** 参数:        none
** 返回:        none
********************************************************************/
void energy_measure_power_store(void);

#endif

