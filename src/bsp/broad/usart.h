/********************************************************************************
**
** 文件名:     usart.h
** 版权所有:   
** 文件描述:   串口配置管理模块
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**|    日期    |  作者  |  修改记录
**===============================================================================
**| 2015/08/01 | 苏友江 |  创建该文件
********************************************************************************/
#ifndef USART_H
#define USART_H



typedef enum 
{
  COM1 = 0,
  COM2 = 1,
  COM3 = 2,
} COM_TypeDef;

#define COMn    3

/**
 * @brief Definition for COM port1, connected to USART1
 */ 
#define EVAL_COM1                        USART1
#define EVAL_COM1_CLK                    RCC_APB2Periph_USART1

#define EVAL_COM1_TX_PIN                 GPIO_Pin_9
#define EVAL_COM1_TX_GPIO_PORT           GPIOA
#define EVAL_COM1_TX_GPIO_CLK            RCC_AHBPeriph_GPIOA
#define EVAL_COM1_TX_SOURCE              GPIO_PinSource9
#define EVAL_COM1_TX_AF                  GPIO_AF_USART1

#define EVAL_COM1_RX_PIN                 GPIO_Pin_10
#define EVAL_COM1_RX_GPIO_PORT           GPIOA
#define EVAL_COM1_RX_GPIO_CLK            RCC_AHBPeriph_GPIOA
#define EVAL_COM1_RX_SOURCE              GPIO_PinSource10
#define EVAL_COM1_RX_AF                  GPIO_AF_USART1

#define EVAL_COM1_CTS_PIN                GPIO_Pin_11
#define EVAL_COM1_CTS_GPIO_PORT          GPIOA
#define EVAL_COM1_CTS_GPIO_CLK           RCC_AHBPeriph_GPIOA
#define EVAL_COM1_CTS_SOURCE             GPIO_PinSource11
#define EVAL_COM1_CTS_AF                 GPIO_AF_USART1

#define EVAL_COM1_RTS_PIN                GPIO_Pin_12
#define EVAL_COM1_RTS_GPIO_PORT          GPIOA
#define EVAL_COM1_RTS_GPIO_CLK           RCC_AHBPeriph_GPIOA
#define EVAL_COM1_RTS_SOURCE             GPIO_PinSource12
#define EVAL_COM1_RTS_AF                 GPIO_AF_USART1
   
#define EVAL_COM1_IRQn                   USART1_IRQn


/**
 * @brief Definition for COM port1, connected to USART2
 */ 
#define EVAL_COM2                        USART2
#define EVAL_COM2_CLK                    RCC_APB1Periph_USART2
#define EVAL_COM2_TX_PIN                 GPIO_Pin_2
#define EVAL_COM2_TX_GPIO_PORT           GPIOA
#define EVAL_COM2_TX_GPIO_CLK            RCC_AHBPeriph_GPIOA
#define EVAL_COM2_TX_SOURCE              GPIO_PinSource2
#define EVAL_COM2_TX_AF                  GPIO_AF_USART2
#define EVAL_COM2_RX_PIN                 GPIO_Pin_3
#define EVAL_COM2_RX_GPIO_PORT           GPIOA
#define EVAL_COM2_RX_GPIO_CLK            RCC_AHBPeriph_GPIOA
#define EVAL_COM2_RX_SOURCE              GPIO_PinSource3
#define EVAL_COM2_RX_AF                  GPIO_AF_USART2
#define EVAL_COM2_IRQn                   USART2_IRQn
#define EVAL_COM2_TX_STREAM          	0
#define EVAL_COM2_RX_STREAM          	0
#define EVAL_COM2_TX_CHANNEL			0
#define EVAL_COM2_RX_CHANNEL          	0
#define EVAL_COM2_TX_IRQ          		0
#define EVAL_COM2_RX_IRQ          		0

#define EVAL_COM2_RTS_PIN                 GPIO_Pin_1
#define EVAL_COM2_RTS_GPIO_PORT           GPIOA
#define EVAL_COM2_RTS_GPIO_CLK            RCC_AHBPeriph_GPIOA
#define EVAL_COM2_RTS_SOURCE              GPIO_PinSource1
#define EVAL_COM2_RTS_AF                  GPIO_AF_USART2
#define EVAL_COM2_CTS_PIN                 GPIO_Pin_0
#define EVAL_COM2_CTS_GPIO_PORT           GPIOA
#define EVAL_COM2_CTS_GPIO_CLK            RCC_AHBPeriph_GPIOA
#define EVAL_COM2_CTS_SOURCE              GPIO_PinSource0
#define EVAL_COM2_CTS_AF                  GPIO_AF_USART2


/**
 * @brief Definition for COM port1, connected to USART3
 */ 
#define EVAL_COM3                        USART3
#define EVAL_COM3_CLK                    RCC_APB1Periph_USART3
#define EVAL_COM3_TX_PIN                 GPIO_Pin_10
#define EVAL_COM3_TX_GPIO_PORT           GPIOC
#define EVAL_COM3_TX_GPIO_CLK            RCC_AHBPeriph_GPIOC
#define EVAL_COM3_TX_SOURCE              GPIO_PinSource10
#define EVAL_COM3_TX_AF                  GPIO_AF_USART3
#define EVAL_COM3_RX_PIN                 GPIO_Pin_11
#define EVAL_COM3_RX_GPIO_PORT           GPIOC
#define EVAL_COM3_RX_GPIO_CLK            RCC_AHBPeriph_GPIOC
#define EVAL_COM3_RX_SOURCE              GPIO_PinSource11
#define EVAL_COM3_RX_AF                  GPIO_AF_USART3
#define EVAL_COM3_IRQn                   USART3_IRQn
#define EVAL_COM3_TX_STREAM          	0
#define EVAL_COM3_RX_STREAM          	0
#define EVAL_COM3_TX_CHANNEL			0
#define EVAL_COM3_RX_CHANNEL          	0
#define EVAL_COM3_TX_IRQ          		0
#define EVAL_COM3_RX_IRQ          		0

#define EVAL_COM3_RTS_PIN                 GPIO_Pin_14
#define EVAL_COM3_RTS_GPIO_PORT           GPIOB
#define EVAL_COM3_RTS_GPIO_CLK            RCC_AHBPeriph_GPIOB
#define EVAL_COM3_RTS_SOURCE              GPIO_PinSource14
#define EVAL_COM3_RTS_AF                  GPIO_AF_USART3
#define EVAL_COM3_CTS_PIN                 GPIO_Pin_13
#define EVAL_COM3_CTS_GPIO_PORT           GPIOB
#define EVAL_COM3_CTS_GPIO_CLK            RCC_AHBPeriph_GPIOB
#define EVAL_COM3_CTS_SOURCE              GPIO_PinSource13
#define EVAL_COM3_CTS_AF                  GPIO_AF_USART3

/*******************************************************************
** 函数名:      usart_com_init
** 函数描述:    串口初始化
** 参数:        [in] com             : 串口号
**              [in] usart_initstruct: 波特率
** 返回:        none
********************************************************************/
void usart_com_init(COM_TypeDef com, USART_InitTypeDef* usart_initstruct);

void usart_configuration(COM_TypeDef com, uint32_t buadrate, uint16_t parity);

void usart_rxit_enable(COM_TypeDef com);

#endif
