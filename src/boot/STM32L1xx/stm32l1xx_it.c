/**
  ******************************************************************************
  * @file    Project/STM32L1xx_StdPeriph_Templates/stm32l1xx_it.c 
  * @author  MCD Application Team
  * @version V1.2.0
  * @date    16-May-2014
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and 
  *          peripherals interrupt service routine.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2014 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32l1xx_it.h"
#include <stdio.h>
#include "common.h"
/** @addtogroup Template_Project
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M3 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief  This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
        SerialPutString(" HardFault_Handler \n\r ");
        NVIC_SystemReset();	
  }
}

#if 0
void hard_fault_handler_c(unsigned int * hardfault_args) 
{
	/* Go to infinite loop when Hard Fault exception occurs */

	unsigned int stacked_r0;  
	unsigned int stacked_r1;  
	unsigned int stacked_r2;  
	unsigned int stacked_r3;  
	unsigned int stacked_r12;  
	unsigned int stacked_lr;  
	unsigned int stacked_pc;  
	unsigned int stacked_psr;  
	
	stacked_r0 = ((unsigned long) hardfault_args[0]);  
	stacked_r1 = ((unsigned long) hardfault_args[1]);  
	stacked_r2 = ((unsigned long) hardfault_args[2]);  
	stacked_r3 = ((unsigned long) hardfault_args[3]);  
	
	stacked_r12 = ((unsigned long) hardfault_args[4]);  
	stacked_lr = ((unsigned long) hardfault_args[5]);  
	stacked_pc = ((unsigned long) hardfault_args[6]);  
	stacked_psr = ((unsigned long) hardfault_args[7]);  
	
	printf ("[Hard fault handler]\r\n");  
	printf("R0 = %x\r\n", stacked_r0);  
	printf ("R1 = %x\r\n", stacked_r1);  
	printf ("R2 = %x\r\n", stacked_r2);  
	printf ("R3 = %x\r\n", stacked_r3);  
	printf ("R12 = %x\r\n", stacked_r12);  
	printf ("LR = %x\r\n", stacked_lr);  
	printf ("PC = %x\r\n", stacked_pc);  
	printf ("PSR = %x\r\n", stacked_psr);  
	printf ("BFAR = %x\r\n", (*((volatile unsigned long *)(0xE000ED38))));  
	printf ("CFSR = %x\r\n", (*((volatile unsigned long *)(0xE000ED28))));  
	printf ("HFSR = %x\r\n", (*((volatile unsigned long *)(0xE000ED2C))));  
	printf("DFSR = %x\r\n", (*((volatile unsigned long *)(0xE000ED30))));  
	printf("AFSR = %x\r\n", (*((volatile unsigned long *)(0xE000ED3C)))); 
	while (1)
	{
        SerialPutString(" hard_fault_handler_c \n\r ");
        NVIC_SystemReset();	
	}
}
#endif /* if 0. 2017-5-3 0:20:08 syj */

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
    SerialPutString(" MemManage_Handler \n\r ");
    NVIC_SystemReset();
  }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
        SerialPutString(" BusFault_Handler \n\r ");
        NVIC_SystemReset();	  
  }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
        SerialPutString(" UsageFault_Handler \n\r ");
        NVIC_SystemReset();	    
  }
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{
}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
}

/******************************************************************************/
/*                 STM32L1xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32l1xx_xx.s).                                            */
/******************************************************************************/

/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval None
  */
/*void PPP_IRQHandler(void)
{
}*/

/**
  * @}
  */ 


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
