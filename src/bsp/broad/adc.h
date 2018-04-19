/********************************************************************************
**
** 文件名:     adc.h
** 版权所有:   (c) 2013-2015 
** 文件描述:   实现ADC驱动
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**|    日期    |  作者  |  修改记录
**===============================================================================
**| 2015/08/31 | 苏友江 |  创建该文件
********************************************************************************/
#ifndef ADC_H
#define ADC_H


#define  SAMPLE_NUM     33

typedef enum {
    AD_CH_LEAKA_V = 0,    /* 采集灯杆漏电压AD通道 */
    AD_CH_LAMPS_V,        /* 采集终端的电压AD通道 */
    AD_CH_LAMP1_I,        /* 采集灯 1的电流AD通道 */
    AD_CH_MAX
} Ad_channle_e;


typedef struct {
	uint8_t   sample;	
	int16_t   buf[AD_CH_MAX][SAMPLE_NUM];	 /* 16bit,3路AD，各采35个 */
}Ad_sample_t;



extern Ad_sample_t g_ad_cvs;



void adc_config(uint8_t ADC_SampleTime_Cycles);

void timer2_init(void);

uint8_t adc_timer_start(void);

uint8_t adc_timer_stop(void);

#endif

