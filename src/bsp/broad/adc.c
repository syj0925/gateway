/********************************************************************************
**
** 文件名:     adc.c
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
#include "stm32l1xx.h"
#include "adc.h"

/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/
static volatile uint32_t s_adc1_buf[AD_CH_MAX] = {0};
static volatile uint8_t  s_dma_flag = 0;
Ad_sample_t g_ad_cvs = {0};

static void STM_EAVL_ADCIni(uint8_t isForChkZero, uint8_t ADC_SampleTime_Cycles)
{
	ADC_InitTypeDef       ADC_InitStructure;
	ADC_CommonInitTypeDef ADC_CommonInitStructure;
	GPIO_InitTypeDef      GPIO_InitStructure;

	/* Enable ADC1, and GPIO clocks ****************************************/   
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);	
	/* ADC Common Init **********************************************************/
	if(isForChkZero==1)
	{	
		ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div1;
	}
	else
		ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div2;
	ADC_CommonInit(&ADC_CommonInitStructure);

	ADC_DeInit(ADC1);
	ADC_StructInit(&ADC_InitStructure);
	ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;		//使能ADC扫描模式
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;	//连续扫描模式
	ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	if(isForChkZero==1)
	{	
		ADC_InitStructure.ADC_NbrOfConversion = 1;
	}
	else
		ADC_InitStructure.ADC_NbrOfConversion = 3;
	ADC_Init(ADC1, &ADC_InitStructure);
//	ADC_ExternalTrigInjectedConvConfig(ADC1, ADC_ExternalTrigInjecConv_T3_CC2);

	/* ADC3 regular channel7 configuration *************************************/
	if(isForChkZero==1)
	{
		ADC_RegularChannelConfig(ADC1, ADC_Channel_12, 1, ADC_SampleTime_Cycles);
	}
	else
	{
		ADC_RegularChannelConfig(ADC1, ADC_Channel_13, 1, ADC_SampleTime_Cycles);
		ADC_RegularChannelConfig(ADC1, ADC_Channel_12, 2, ADC_SampleTime_Cycles);
		ADC_RegularChannelConfig(ADC1, ADC_Channel_11, 3, ADC_SampleTime_Cycles);
	}
	
	ADC_Cmd(ADC1, ENABLE);
}

static void _tim2_config(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;

	/* TIM3 clock enable */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

	/* Enable the TIM2 gloabal Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

/**
  * @brief  This function handles DMA Transfer Complete interrupt request.
  * @param  None
  * @retval None
  */
void DMA1_Channel1_IRQHandler(void)
{
	uint8_t  i;
    
	if (DMA_GetITStatus(DMA1_IT_TC1) == SET) {
		DMA_ITConfig(DMA1_Channel1, DMA1_IT_TC1, DISABLE);
		if (s_dma_flag == 1) {
			for (i = 0; i < AD_CH_MAX; i++) {
				g_ad_cvs.buf[i][g_ad_cvs.sample] = (s_adc1_buf[i]) & 0xfff;
			}
			g_ad_cvs.sample++;
			if (g_ad_cvs.sample >= SAMPLE_NUM) {
            	adc_timer_stop();
			}
			s_dma_flag = 0;
		}
		DMA_ClearITPendingBit(DMA1_IT_TC1);	
	}
}

void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET) {
        if (s_dma_flag == 0) {
            s_dma_flag = 1;
    		ADC_DMACmd(ADC1, ENABLE);
    		ADC_Cmd(ADC1, ENABLE);
    		ADC_SoftwareStartConv(ADC1);
    		ADC_DMARequestAfterLastTransferCmd(ADC1, ENABLE);
    		DMA_ITConfig(DMA1_Channel1, DMA1_IT_TC1, ENABLE);            
        }
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}
}

/*******************************************************************
** 函数名:      adc_config
** 函数描述:    ADC初始化，注：必须配置HSI，否则无法DMA终端
** 参数:        [in] ADC_SampleTime_Cycles
** 返回:        none
********************************************************************/
void adc_config(uint8_t ADC_SampleTime_Cycles)
{

	DMA_InitTypeDef  DMA_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	/* Enable ADC1, and GPIO clocks ****************************************/
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1 | RCC_AHBPeriph_GPIOC, ENABLE);     
	//RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

	DMA_DeInit(DMA1_Channel1);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)s_adc1_buf;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_BufferSize = 3;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel1, &DMA_InitStructure);
	DMA_Cmd(DMA1_Channel1, ENABLE);

	//NVIC_PriorityGroupConfig(NVIC_PriorityGroup_3);
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel1_IRQn;
  	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;
  	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;
  	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  	NVIC_Init(&NVIC_InitStructure);
	
	/*enable transfer complete interrupt------------------*/
	//DMA_ITConfig(DMA1_Channel1, DMA1_IT_TC1, ENABLE); 
	
	STM_EAVL_ADCIni(0, ADC_SampleTime_Cycles);
	
	ADC_DMARequestAfterLastTransferCmd(ADC1, ENABLE);
	ADC_DMACmd(ADC1, ENABLE);	
	//ADC_SoftwareStartConv(ADC1);
}


/*******************************************************************
** 函数名:      timer2_init
** 函数描述:    通用定时器2初始化，用于AD采样计时
** 参数:        none
** 返回:        none
********************************************************************/
void timer2_init(void)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	unsigned int val;
	unsigned int psc;
	unsigned int arr;
	RCC_ClocksTypeDef RCC_Clocks;
	
	RCC_GetClocksFreq(&RCC_Clocks);
	
	_tim2_config();

    /* 交流电50Hz，周期为20ms，一个周期取32个点，采样周期：625us，因此定时器溢出时间为625us */
    /*    
    Tout= ((arr+1)*(psc+1))/Tclk；
    其中
    Tclk：TIM3的输入时钟频率（单位为Mhz）。 
    Tout：TIM3溢出时间（单位为 us）。
    */
#if 1
	val = RCC_Clocks.PCLK1_Frequency / (50 * 32);
	psc = (val + 49999) / 50000 - 1;
	arr = (val / (psc + 1)) - 1;
#endif /* if 0. 2015-9-7 11:10:51 syj */
#if 0
	psc = 3999;
	arr = 4;
#endif /* if 0. 2015-9-7 11:32:49 syj */
    
	/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Period = arr;                                    /* 装载值 */
	TIM_TimeBaseStructure.TIM_Prescaler = psc;                                 /* 分频值 */
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

	/* Prescaler configuration */
	TIM_PrescalerConfig(TIM2, psc, TIM_PSCReloadMode_Update);

	TIM_SetAutoreload(TIM2, arr);

	/* TIM Interrupts enable */
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

	/* TIM2 disable counter */
	TIM_Cmd(TIM2, DISABLE);
}

uint8_t adc_timer_start(void)
{
	uint8_t ret = 0;

	/* TIM Interrupts enable */
	//TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

	/* TIM2 enable counter */
	TIM_Cmd(TIM2, ENABLE);
 
	return ret;
}

uint8_t adc_timer_stop(void)
{
	uint8_t ret = 0;

	//TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE);
	TIM_Cmd(TIM2, DISABLE);
 
	return ret;
}

