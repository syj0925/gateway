/********************************************************************************
**
** 文件名:     usart.c
** 版权所有:   (c) 2013-2015 
** 文件描述:   串口配置管理模块
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**|    日期    |  作者  |  修改记录
**===============================================================================
**| 2015/08/01 | 苏友江 |  创建该文件
********************************************************************************/
#include "stm32l1xx.h"  
#include "usart.h"


USART_TypeDef* COM_USART[COMn] = {EVAL_COM1, EVAL_COM2, EVAL_COM3}; 

GPIO_TypeDef* COM_TX_PORT[COMn] = {EVAL_COM1_TX_GPIO_PORT,EVAL_COM2_TX_GPIO_PORT,EVAL_COM3_TX_GPIO_PORT};
 
GPIO_TypeDef* COM_RX_PORT[COMn] = {EVAL_COM1_RX_GPIO_PORT,EVAL_COM2_RX_GPIO_PORT,EVAL_COM3_RX_GPIO_PORT};

const uint32_t COM_USART_CLK[COMn] = {EVAL_COM1_CLK,EVAL_COM2_CLK,EVAL_COM3_CLK};

const uint32_t COM_TX_PORT_CLK[COMn] = {EVAL_COM1_TX_GPIO_CLK,EVAL_COM2_TX_GPIO_CLK,EVAL_COM3_TX_GPIO_CLK};
 
const uint32_t COM_RX_PORT_CLK[COMn] = {EVAL_COM1_RX_GPIO_CLK,EVAL_COM2_RX_GPIO_CLK,EVAL_COM3_RX_GPIO_CLK};

const uint16_t COM_TX_PIN[COMn] = {EVAL_COM1_TX_PIN,EVAL_COM2_TX_PIN,EVAL_COM3_TX_PIN};

const uint16_t COM_RX_PIN[COMn] = {EVAL_COM1_RX_PIN,EVAL_COM2_RX_PIN,EVAL_COM3_RX_PIN};
 
const uint16_t COM_TX_PIN_SOURCE[COMn] = {EVAL_COM1_TX_SOURCE,EVAL_COM2_TX_SOURCE,EVAL_COM3_TX_SOURCE};

const uint16_t COM_RX_PIN_SOURCE[COMn] = {EVAL_COM1_RX_SOURCE,EVAL_COM2_RX_SOURCE,EVAL_COM3_RX_SOURCE};
 
const uint16_t COM_TX_AF[COMn] = {EVAL_COM1_TX_AF,EVAL_COM2_TX_AF,EVAL_COM3_TX_AF};
 
const uint16_t COM_RX_AF[COMn] = {EVAL_COM1_RX_AF,EVAL_COM2_RX_AF,EVAL_COM3_RX_AF};

const uint8_t COM_IRQ[COMn] = {EVAL_COM1_IRQn,EVAL_COM2_IRQn,EVAL_COM3_IRQn};


static void STM_EVAL_COM_NVIC_Config(COM_TypeDef COM, uint8_t en)
{
  NVIC_InitTypeDef NVIC_InitStructure;

  /* Enable the USARTx Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = COM_IRQ[COM];
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x03;
  NVIC_InitStructure.NVIC_IRQChannelCmd = (FunctionalState)en;
  NVIC_Init(&NVIC_InitStructure);
}

/*******************************************************************
** 函数名:      usart_com_init
** 函数描述:    串口初始化
** 参数:        [in] com             : 串口号
**              [in] usart_initstruct: 波特率
** 返回:        none
********************************************************************/
void usart_com_init(COM_TypeDef com, USART_InitTypeDef* usart_initstruct)
{
    GPIO_InitTypeDef GPIO_InitStructure;
        
    /* Enable GPIO clock */
    RCC_AHBPeriphClockCmd(COM_TX_PORT_CLK[com] | COM_RX_PORT_CLK[com], ENABLE);

    /* Enable USART clock */
    if (com == COM1)
        RCC_APB2PeriphClockCmd(COM_USART_CLK[com], ENABLE); 
    else if (com == COM2)
        RCC_APB1PeriphClockCmd(COM_USART_CLK[com], ENABLE); 
    else if (com == COM3)
        RCC_APB1PeriphClockCmd(COM_USART_CLK[com], ENABLE);   

    /* Connect PXx to USARTx_Tx */
    GPIO_PinAFConfig(COM_TX_PORT[com], COM_TX_PIN_SOURCE[com], COM_TX_AF[com]);

    /* Connect PXx to USARTx_Rx */
    GPIO_PinAFConfig(COM_RX_PORT[com], COM_RX_PIN_SOURCE[com], COM_RX_AF[com]);

    /* Configure USART Tx as alternate function push-pull */
    GPIO_InitStructure.GPIO_Pin   = COM_TX_PIN[com];
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_Init(COM_TX_PORT[com], &GPIO_InitStructure);

    /* Configure USART Rx as alternate function push-pull */
    GPIO_InitStructure.GPIO_Pin = COM_RX_PIN[com];
    GPIO_Init(COM_RX_PORT[com], &GPIO_InitStructure);
    
    /* Configure USART1 */
    USART_Init(COM_USART[com], usart_initstruct);
    /* Enable USART1 Receive and Transmit interrupts */
    //USART_ITConfig(COM_USART[com], USART_IT_RXNE, ENABLE);
    /* Enable the USART1 */
    USART_Cmd(COM_USART[com], ENABLE);
}

void usart_configuration(COM_TypeDef com, uint32_t buadrate, uint16_t parity)
{
	USART_InitTypeDef USART_InitStructure;

	USART_InitStructure.USART_BaudRate = buadrate;
	if (parity == USART_Parity_No) {
        USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    } else {
		USART_InitStructure.USART_WordLength = USART_WordLength_9b;
    }
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = parity;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	
	STM_EVAL_COM_NVIC_Config(com, ENABLE);
	usart_com_init(com, &USART_InitStructure);

    if (com == COM1) {
    	USART_ITConfig(EVAL_COM1, USART_IT_RXNE, DISABLE);
    	//USART_ClearFlag(EVAL_COM1, USART_FLAG_TC);
    } else if (com == COM2) {
    	USART_ITConfig(EVAL_COM2, USART_IT_RXNE, DISABLE);
    	//USART_ClearFlag(EVAL_COM2, USART_FLAG_TC);
    } else if (com == COM3) {
    	USART_ITConfig(EVAL_COM3, USART_IT_RXNE, DISABLE);
    	//USART_ClearFlag(EVAL_COM3, USART_FLAG_TC);
    }
}

void usart_rxit_enable(COM_TypeDef com)
{
    if (com == COM1) {
    	USART_ITConfig(EVAL_COM1, USART_IT_RXNE, ENABLE);
    	//USART_ClearFlag(EVAL_COM1, USART_FLAG_TC);
    } else if (com == COM2) {
    	USART_ITConfig(EVAL_COM2, USART_IT_RXNE, ENABLE);
    	//USART_ClearFlag(EVAL_COM2, USART_FLAG_TC);
    } else if (com == COM3) {
    	USART_ITConfig(EVAL_COM3, USART_IT_RXNE, ENABLE);
    	//USART_ClearFlag(EVAL_COM3, USART_FLAG_TC);
    }
}

