/********************************************************************************
**
** 文件名:     spi_w5500.c
** 版权所有:   (c) 2013-201 
** 文件描述:   SPI驱动函数实现
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**|    日期    |  作者  |  修改记录
**===============================================================================
**| 2015/06/12 | 苏友江 |  创建该文件
********************************************************************************/
#include "stm32l1xx.h"  
#include "stm32l1xx_spi.h"
#include "sys_swconfig.h"
#if EN_ETHERNET > 0

static void delay_us(uint32_t time)
{
    uint16_t i = 0;

    while(time--) {
        i = 10;                                                                /* 根据实际情况进行调整 */
        while(i--);
    }
}

static void delay_ms(uint16_t time)
{
    uint16_t i = 0;

    while(time--) {
        i = 12000;                                                             /* 根据实际情况进行调整 */
        while(i--);
    }
}

/**
  * @brief  使能SPI时钟
  * @retval None
  */
static void SPI_RCC_Configuration(void)
{
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB | RCC_AHBPeriph_GPIOC, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
}

/**
  * @brief  配置指定SPI的引脚
  * @retval None
  */
static void SPI_GPIO_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;


    /* PB12->CS,PB13->SCK,PB14->MISO,PB15->MOSI */
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_40MHz;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    /* 初始化片选输出引脚 */
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_12;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_40MHz;
    GPIO_Init(GPIOB, &GPIO_InitStruct);
    GPIO_SetBits(GPIOB, GPIO_Pin_12);

    /* 初始w5500reset输出引脚 */
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_40MHz;
    GPIO_Init(GPIOC, &GPIO_InitStruct);
    GPIO_SetBits(GPIOC, GPIO_Pin_6);

    /* GPIO_AF_SPI2 */
    /* Connect PXx to sEE_SPI_SCK */
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource13, GPIO_AF_SPI2);

    /* Connect PXx to sEE_SPI_MISO */
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource14, GPIO_AF_SPI2); 

    /* Connect PXx to sEE_SPI_MOSI */
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource15, GPIO_AF_SPI2); 
}

/**
  * @brief  根据外部SPI设备配置SPI相关参数
  * @retval None
  */
void spi_configuration(void)
{
    SPI_InitTypeDef SPI_InitStruct;
 
    SPI_RCC_Configuration();
 
    SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
    SPI_InitStruct.SPI_Direction= SPI_Direction_2Lines_FullDuplex;
    SPI_InitStruct.SPI_Mode = SPI_Mode_Master;
    SPI_InitStruct.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStruct.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStruct.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStruct.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStruct.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStruct.SPI_CRCPolynomial = 7;
    SPI_Init(SPI2,&SPI_InitStruct);
     
    SPI_GPIO_Configuration();
 
    SPI_SSOutputCmd(SPI2, ENABLE);
    SPI_Cmd(SPI2, ENABLE);
}

/**
  * @brief  写1字节数据到SPI总线
  * @param  byte 写到总线的数据
  * @retval None
  */
void spi_write_byte(uint8_t byte)
{          

    while((SPI2->SR&SPI_I2S_FLAG_TXE)==0);                                     /* 等待发送区空          */
    SPI2->DR = byte;                                                           /* 发送一个byte  */
    while((SPI2->SR&SPI_I2S_FLAG_RXNE)==0);                                    /* 等待接收完一个byte  */
    SPI2->DR;        
}
/**
  * @brief  从SPI总线读取1字节数据
  * @retval 读到的数据
  */
uint8_t spi_read_byte(void)
{   
    while((SPI2->SR&SPI_I2S_FLAG_TXE)==0);                                     /* 等待发送区空              */
    SPI2->DR=0xFF;                                                             /* 发送一个空数据产生输入数据的时钟  */
    while((SPI2->SR&SPI_I2S_FLAG_RXNE)==0);                                    /* 等待接收完一个byte  */
    return SPI2->DR;                             
}
/**
  * @brief  进入临界区
  * @retval None
  */
void spi_crisenter(void)
{
    __set_PRIMASK(1);
}
/**
  * @brief  退出临界区
  * @retval None
  */
void spi_crisexit(void)
{
    __set_PRIMASK(0);
}
 
/**
  * @brief  片选信号输出低电平
  * @retval None
  */
void spi_cs_select(void)
{
    GPIO_ResetBits(GPIOB,GPIO_Pin_12);
}
/**
  * @brief  片选信号输出高电平
  * @retval None
  */
void spi_cs_deselect(void)
{
    GPIO_SetBits(GPIOB,GPIO_Pin_12);
}

void reset_w5500(void)
{
  GPIO_ResetBits(GPIOC, GPIO_Pin_6);
  delay_us(100);
  GPIO_SetBits(GPIOC, GPIO_Pin_6);
  delay_ms(200);
}
#endif /* end of EN_ETHERNET */
/*********************************END OF FILE**********************************/

