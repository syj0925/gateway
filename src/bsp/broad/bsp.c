/********************************************************************************
**
** 文件名:     bsp.c
** 版权所有:   
** 文件描述:   管理板子运行所需的硬件资源的配置、初始化等
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**|    日期    |  作者  |  修改记录
**===============================================================================
**| 2015/06/12 | 苏友江 |  创建该文件
********************************************************************************/
#include "bsp.h"
#include <stdio.h>
#include "sys_typedef.h"
#include "sys_swconfig.h"
#include "sys_debugcfg.h"

#if DBG_SYSTEM > 0
#define SYS_DEBUG          OS_DEBUG
#else
#define SYS_DEBUG(...)     do{}while(0)
#endif

#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif

PUTCHAR_PROTOTYPE
{
	if (ch == '\n') {
		uint8_t rch = '\r';

		while (USART_GetFlagStatus(EVAL_COM1, USART_FLAG_TC) == RESET);
		USART_SendData(EVAL_COM1, (uint8_t) rch);
	}

	while (USART_GetFlagStatus(EVAL_COM1, USART_FLAG_TC) == RESET);	
	USART_SendData(EVAL_COM1, (uint8_t) ch);
	
	return ch;
}

#if 1
int fgetc(FILE *f)
{
    /* Loop until received a char */
    while(!(USART_GetFlagStatus(EVAL_COM1, USART_FLAG_RXNE) == SET));
    /* Read a character from the USART and RETURN */
    return (USART_ReceiveData(EVAL_COM1));
}
#endif /* if 0. 2016-3-12 下午 8:31:56 syj */

#if 0
/*******************************************************************
** 函数名:      _rcc_configuration
** 函数描述:    RCC配置
** 参数:        none
** 返回:        none
********************************************************************/
static void _rcc_configuration(void)
{   
  /* Enable HSI Clock */
  RCC_HSICmd(ENABLE);
  
  /*!< Wait till HSI is ready */
  while (RCC_GetFlagStatus(RCC_FLAG_HSIRDY) == RESET)
  {}

  RCC_SYSCLKConfig(RCC_SYSCLKSource_HSI);
  
  RCC_MSIRangeConfig(RCC_MSIRange_6);
		
  /* Enable the GPIOs clocks */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA | RCC_AHBPeriph_GPIOB | RCC_AHBPeriph_GPIOC| RCC_AHBPeriph_GPIOD| RCC_AHBPeriph_GPIOE| RCC_AHBPeriph_GPIOH, ENABLE);     

  RCC_HSEConfig(RCC_HSE_OFF);  
  if(RCC_GetFlagStatus(RCC_FLAG_HSERDY) != RESET )
  {
    while(1);
  }
}
#endif /* if 0. 2015-8-31 09:51:41 syj */

#if 0
/*******************************************************************
** 函数名:      _debug_usart_configuration
** 函数描述:    调试串口初始化
** 参数:        [in] usartx  : 串口号
**              [in] baudRate: 波特率
** 返回:        none
********************************************************************/
static void _debug_usart_configuration(COM_TypeDef com, uint32_t baudRate)
{
    USART_InitTypeDef USART_InitStructure;

    USART_InitStructure.USART_BaudRate = baudRate;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

    usart_com_init(com, &USART_InitStructure);
}
#endif /* if 0. 2016-3-12 下午 2:31:55 syj */

/*******************************************************************
** 函数名:      _watchdog_Init
** 函数描述:    初始化独立看门狗
** 参数:        none
** 返回:        none
********************************************************************/
static void _watchdog_Init(void)
{
    RCC_LSICmd(ENABLE);                                                        /* 打开LSI */
    while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY)==RESET);                          /* 等待直到LSI稳定 */

    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);                              /* 打开使能，因为iwdg的寄存器有写保护,必须先写入0x5555，才能操作寄存器 */
    IWDG_SetPrescaler(IWDG_Prescaler_256);                                     /* 独立看门狗使用内部低速振荡器LSI，对LSI进行256分频 */
    IWDG_SetReload(1023);                                                      /* 设定独立看门狗计数器的计数值(0x000~0xFFF;0~4095)，复位时间为6s */
    IWDG_ReloadCounter();                                                      /* 重载独立看门狗计数器，向寄存器写入0xAAAA，或者更新计数值 */
    IWDG_Enable();                                                             /* 开启看门狗，向寄存器写入0xCCCC即可 */
}

/**
  * @brief  Configures the RTC Alarm.
  * @param  None
  * @retval None
  */
static void _rtc_alarm_config(void)
{
    EXTI_InitTypeDef EXTI_InitStructure;
    RTC_AlarmTypeDef RTC_AlarmStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    /* EXTI configuration */
    EXTI_ClearITPendingBit(EXTI_Line17);
    EXTI_InitStructure.EXTI_Line = EXTI_Line17;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    /* Clear the RTC Alarm Flag */
    RTC_ClearFlag(RTC_FLAG_ALRAF);
    /* Enable the RTC Alarm Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = RTC_Alarm_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* Set the alarmA Masks */
    RTC_AlarmStructure.RTC_AlarmMask = RTC_AlarmMask_All;
    RTC_SetAlarm(RTC_Format_BIN, RTC_Alarm_A, &RTC_AlarmStructure);

    /* Set AlarmA subseconds and enable SubSec Alarm : generate 8 interripts per Second */
    RTC_AlarmSubSecondConfig(RTC_Alarm_A, 0xFF, RTC_AlarmSubSecondMask_SS14_5);

    /* Enable AlarmA interrupt */
    RTC_ITConfig(RTC_IT_ALRA, ENABLE);

    /* Enable the alarmA */
    RTC_AlarmCmd(RTC_Alarm_A, ENABLE);
}

/*******************************************************************
** 函数名:      _rtc_init
** 函数描述:    内部实时时钟初始化
** 参数:        none
** 返回:        none
********************************************************************/
static void _rtc_init(void)
{
    RTC_InitTypeDef RTC_InitStructure;

    RTC_DeInit();
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

    PWR_RTCAccessCmd(ENABLE);
#if 1                                                                          /* 使用外部晶振 */
	RCC_LSEConfig(RCC_LSE_ON);
	while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET);
	RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
#else                                                                          /* 使用内部rc晶振,在没有接外部晶振，采用内部RC振荡差不多40分钟会误差个2分钟左右 */
    RCC_LSICmd(ENABLE);
	while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET);
	RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
#endif

    RCC_RTCCLKCmd(ENABLE);
    RTC_WaitForSynchro();

    RTC_StructInit(&RTC_InitStructure);

    RTC_Init(&RTC_InitStructure);

    /* 秒中断参考STM32L1xx_StdPeriph_Lib_V1.3.1\Project\STM32L1xx_StdPeriph_Examples\RTC\RTC_Timer例程 */
    /* 按照例程设置：1s八次中断，但是STM32L100RBT6却是1秒一次中断，暂不知道原因 */
    /* 使用stm32l100rct6板子测试，是1s八次中断 */
    _rtc_alarm_config();                                                       /* 设置RTC秒中断，针对一些对时间要求高的要求 */
}

/*******************************************************************
** 函数名:      _led_init
** 函数描述:    led初始化
** 参数:        无
** 返回:        无
********************************************************************/
void _led_init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;

    /* Enable the GPIO_LED Clock */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);

    /* Configure the GPIO_LED pin */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

static void _lamp_gpio_init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;

    /* Enable the GPIO Clock */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);

    /* Configure the GPIO pin */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4|GPIO_Pin_5;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
   
    GPIO_Init(GPIOC, &GPIO_InitStructure);
}

#if EN_DALI > 0
static void _nvic_tim6_config(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;

	/* TIM9 clock enable */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);

	/* Enable the TIM9 gloabal Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = TIM6_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

static void _tim6_config(void)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	unsigned int baud_val;
	unsigned int psc;
	unsigned int arr;
	RCC_ClocksTypeDef RCC_Clocks;
	
	RCC_GetClocksFreq(&RCC_Clocks);
	
	_nvic_tim6_config();
	//------------------------------------------------------------------------
	//time=(psc+1)*(arr+1)/PCLK2=1/baud	
	//------------------------------------------------------------------------	
	baud_val = 9600; // 1200*8 because dali tick 
	arr = RCC_Clocks.PCLK2_Frequency/baud_val-1;
	psc = 0;	

	/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Period = arr;
	TIM_TimeBaseStructure.TIM_Prescaler = psc;//;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

	TIM_TimeBaseInit(TIM6, &TIM_TimeBaseStructure);

	/* Prescaler configuration */
	TIM_PrescalerConfig(TIM6, psc, TIM_PSCReloadMode_Update);

	TIM_SetAutoreload(TIM6, arr);

	/* TIM Interrupts enable */
	TIM_ITConfig(TIM6, TIM_IT_Update, ENABLE);

	/* TIM9 enable counter */
	TIM_Cmd(TIM6, ENABLE);
}

static void _dali_gpio_init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;

    /* Enable the GPIO Clock */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);

    /* Configure the GPIO pin */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_4;                                /* DALI TX */
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
   
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_5;                                /* DALI RX */
  	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_SetBits(GPIOA, GPIO_Pin_4);
}

void _dali_init(void)
{
	_tim6_config();
    _dali_gpio_init();
#if 0
	STM_EVAL_GPIOInit(GPIO_DALI_TX);
	STM_EVAL_GPIOInit(GPIO_DALI_RX);
	XtBspDOCtrl(XT_BSP_DALI_TX, XT_BSP_DO_OP_OPEN);
#endif /* if 0. 2016-3-17 下午 10:06:12 syj */
}
#endif

/*******************************************************************
** 函数名:      bsp_init
** 函数描述:    板级资源配置、初始化，为系统层提供最基本的运行环境
** 参数:        none
** 返回:        none
********************************************************************/
void bsp_init(void)
{
    /* 喂狗超时设置为6s */
    _watchdog_Init();
    bsp_watchdog_feed();
    /* 根据需要再配置,stm库程序已经有配置好 */
    //_rcc_configuration();

    /* 配置系统定时器，用于定时器tick，24位倒计寄存器 */
    SysTick_Config(SystemCoreClock / 200);                                     /* /1000 = 1ms, /100 = 10ms */
    /* 调试串口配置，用于printf */
    //_debug_usart_configuration(COM1, 115200);                                /* 只是用于printf，不开启接收中断 */
    usart_configuration(DEBUG_UART_NO, DEBUG_UART_BAUD, USART_Parity_No);      /* 用于printf，并且开启接收中断 */

    usart_configuration(COM2, 115200, USART_Parity_No);                        /* ZIGBEE/PLC模块通讯 */
    usart_configuration(COM3, 9600, USART_Parity_No);                          /* TTS语音 */

    /* 初始化调光模块 */
    #if EN_DALI > 0
    _dali_init();
    #else
    dimming_init();    
    #endif
    
    /* lamp gpio 初始化 */
    _lamp_gpio_init();

    /* 电能ADC采样初始化 */
	adc_config(ADC_SampleTime_16Cycles);
    timer2_init();
    
    /* 实时时钟初始化,用于提供日历 */
    _rtc_init();

    ds1338_init();                                                             /* 外部时钟，由于内部RTC掉电时间会丢失，因此需要外部时钟辅助 */
        
    _led_init();

#if 0
    if (SCB->VTOR == APP_START_ADDR) {

    } else if (SCB->VTOR == (APP_START_ADDR + APP_MAX_SIZE)) {

    } else {

    }
#endif /* if 0. 2017-4-19 20:31:36 syj */
    bsp_watchdog_feed();
    SYS_DEBUG("<boot app addr:0x%x ver:%s>\r\n", SCB->VTOR, SOFTWARE_VERSION_STR);
    {
    	RCC_ClocksTypeDef RCC_Clocks;
        SystemCoreClockUpdate();
    	RCC_GetClocksFreq(&RCC_Clocks);
        SYS_DEBUG("<SystemCoreClock :%d>\r\n", SystemCoreClock);
    	SYS_DEBUG("<SYSCLK_Frequency:%d>\r\n", RCC_Clocks.SYSCLK_Frequency);
    	SYS_DEBUG("<HCLK_Frequency  :%d>\r\n", RCC_Clocks.HCLK_Frequency);
    	SYS_DEBUG("<PCLK1_Frequency :%d>\r\n", RCC_Clocks.PCLK1_Frequency);
    	SYS_DEBUG("<PCLK2_Frequency :%d>\r\n", RCC_Clocks.PCLK2_Frequency);
    }
}

/*******************************************************************
** 函数名:      bsp_watchdog_feed
** 函数描述:    看门狗喂狗函数
** 参数:        none
** 返回:        none
********************************************************************/
void bsp_watchdog_feed(void)
{
	IWDG_ReloadCounter();
}

#if EN_DALI > 0
/**
  * @brief  Configure PA7 in interrupt mode
  * @param  None
  * @retval None
  */
void bsp_ext9_5_config(void)
{
    EXTI_InitTypeDef   EXTI_InitStructure;
    NVIC_InitTypeDef   NVIC_InitStructure;
    //PrintfXTOS("enRx\n");
    /* Enable SYSCFG clock */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
    /* Connect EXTI8 Line to PA7 pin */
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource7);


    /* Configure EXTI8 line */
    EXTI_InitStructure.EXTI_Line = EXTI_Line7;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    /* Enable and set EXTI9_5 Interrupt to the lowest priority */
    NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

    NVIC_Init(&NVIC_InitStructure);
}

void bsp_ext9_5_deconfig(void)
{
    EXTI_InitTypeDef   EXTI_InitStructure;
    NVIC_InitTypeDef   NVIC_InitStructure;
    //PrintfXTOS("disRx\n");
    /* Enable SYSCFG clock */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, DISABLE);
    /* Connect EXTI8 Line to PA7 pin */
    //SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource7);

    /* Configure EXTI8 line */
    EXTI_InitStructure.EXTI_Line = EXTI_Line7;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = DISABLE;
    EXTI_Init(&EXTI_InitStructure);

    /* Enable and set EXTI9_5 Interrupt to the lowest priority */
    NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
    NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;

    NVIC_Init(&NVIC_InitStructure);
}
#endif

