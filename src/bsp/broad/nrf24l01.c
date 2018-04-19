/********************************************************************************
**
** 文件名:     nrf24l01p.c
** 版权所有:   
** 文件描述:   实现nrf24l01p驱动
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2012/08/14 | 苏友江 |  创建该文件
*********************************************************************************/
#include "bsp.h"
#include "sys_includes.h"
#include "nrf24l01.h"

#if EN_NRF24L01 > 0
/*
********************************************************************************
* NRF24L01寄存器操作命令
********************************************************************************
*/
#define NRF_READ_REG    0x00                                                   /* 读配置寄存器,低5位为寄存器地址 */
#define NRF_WRITE_REG   0x20                                                   /* 写配置寄存器,低5位为寄存器地址 */
#define RD_RX_PLOAD     0x61                                                   /* 读RX有效数据,1~32字节 */
#define WR_TX_PLOAD     0xA0                                                   /* 写TX有效数据,1~32字节 */
#define FLUSH_TX        0xE1                                                   /* 清除TX FIFO寄存器.发射模式下用 */
#define FLUSH_RX        0xE2                                                   /* 清除RX FIFO寄存器.接收模式下用 */
#define REUSE_TX_PL     0xE3                                                   /* 重新使用上一包数据,CE为高,数据包被不断发送. */
#define W_ACK_PAYLOAD   0xA8
#define W_TX_PAYLOAD_NO_ACK 0xB0

#define NOP             0xFF                                                   /* 空操作,可以用来读状态寄存器	  */

/*
********************************************************************************
* NRF24L01寄存器地址
********************************************************************************
*/
#define CONFIG          0x00                                                   /* 配置寄存器地址                             */
#define EN_AA           0x01                                                   /* 使能自动应答功能  */
#define EN_RXADDR       0x02                                                   /* 接收地址允许 */
#define SETUP_AW        0x03                                                   /* 设置地址宽度(所有数据通道) */
#define SETUP_RETR      0x04                                                   /* 建立自动重发 */
#define RF_CH           0x05                                                   /* RF通道 */
#define RF_SETUP        0x06                                                   /* RF寄存器 */
#define STATUS          0x07                                                   /* 状态寄存器 */
#define OBSERVE_TX      0x08                                                   /* 发送检测寄存器 */
#define CD              0x09                                                   /* 载波检测寄存器 */
#define RX_ADDR_P0      0x0A                                                   /* 数据通道0接收地址 */
#define RX_ADDR_P1      0x0B                                                   /* 数据通道1接收地址 */
#define RX_ADDR_P2      0x0C                                                   /* 数据通道2接收地址 */
#define RX_ADDR_P3      0x0D                                                   /* 数据通道3接收地址 */
#define RX_ADDR_P4      0x0E                                                   /* 数据通道4接收地址 */
#define RX_ADDR_P5      0x0F                                                   /* 数据通道5接收地址 */
#define TX_ADDR         0x10                                                   /* 发送地址寄存器 */
#define RX_PW_P0        0x11                                                   /* 接收数据通道0有效数据宽度(1~32字节) */
#define RX_PW_P1        0x12                                                   /* 接收数据通道1有效数据宽度(1~32字节) */
#define RX_PW_P2        0x13                                                   /* 接收数据通道2有效数据宽度(1~32字节) */
#define RX_PW_P3        0x14                                                   /* 接收数据通道3有效数据宽度(1~32字节) */
#define RX_PW_P4        0x15                                                   /* 接收数据通道4有效数据宽度(1~32字节) */
#define RX_PW_P5        0x16                                                   /* 接收数据通道5有效数据宽度(1~32字节) */
#define FIFO_STATUS     0x17                                                   /* FIFO状态寄存器 */
#define FEATURE         0x1D                                                   /* 特征寄存器 */

/*
********************************************************************************
* 配置参数定义
********************************************************************************
*/
#define PLOAD_WIDTH      32

/* 配置寄存器CONFIG相关参数 */
#define MASK_RX_DR   0x40                                                      /* 接收中断屏蔽控制, 0 ：接收中断使能;1 ：接收中断关闭 */
#define MASK_TX_DS   0x20                                                      /* 发射中断屏蔽控制 */
#define MASK_MAX_RT  0x10                                                      /* 最大重发计数中断屏蔽控制 */
#define EN_CRC       0x08                                                      /* 0 ：关闭CRC;1 ：开启CRC */
#define CRCO         0x04                                                      /* CRC长度配置， 0 ：1byte;1 ：2 bytes */
#define PWR_UP       0x02                                                      /* 关断/开机模式配置;0 ：关断模式;1 ：开机模式 */
#define PRIM_RX      0x01                                                      /* 发射/接收配置，1 ：接收模式 ;0 ：发射模式;只能在Shutdown和Standby下更新 */

#define DEF_COFIG    MASK_MAX_RT | EN_CRC | CRCO                               /* 开启接收/发送中断，使能CRC校验，CRC为2byte */

/* 配置寄存器SETUP_AW相关参数 */
#define AW           3
#define AW_3         0x01                                                      /* 01:3bytes */
#define AW_4         0x10                                                      /* 10:4bytes */
#define AW_5         0x11                                                      /* 11:5bytes */

/* STATUS寄存器bit位定义 */
#define TX_OK   	0x20                                                       /* TX发送完成中断*/
#define RX_OK   	0x40  	                                                   /* 接收到数据中断 */
/*
********************************************************************************
* 结构定义
********************************************************************************
*/

typedef struct {
    WORK_MODE_E curmode;
    INT8U src_addr[AW];
    INT8U dest_addr[AW];
} PCB_T;

/*
********************************************************************************
* 静态变量
********************************************************************************
*/
static PCB_T s_pcb = {
    INVALID,
    {0x99, 0x99, 0x99},
    {0x99, 0x99, 0x99}     
};


//24L01操作线
#define NRF24L01_CE(x)   _spi_ce_select(x)  /* 24L01片选信号 */
#define NRF24L01_CSN(x)  _spi_csn_select(x) /* SPI片选信号 */   

static void _spi_ce_select(INT8U value)
{
    if (value) {
        GPIO_SetBits(GPIOB, GPIO_Pin_1);
    } else {
        GPIO_ResetBits(GPIOB, GPIO_Pin_1);
    }
}

static void _spi_csn_select(INT8U value)
{
    if (value) {
        GPIO_SetBits(GPIOB, GPIO_Pin_12);
    } else {
        GPIO_ResetBits(GPIOB, GPIO_Pin_12);
    }
}

/**
  * @brief  配置指定SPI的引脚
  * @retval None
  */
static void _spi_gpio_config(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    /* PB13->SCK,PB14->MISO,PB15->MOSI */
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_40MHz;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    /* PB12->CSN 初始化片选输出引脚，PB1->CE初始化CE输出引脚 */
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_1;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_40MHz;
    GPIO_Init(GPIOB, &GPIO_InitStruct);
    GPIO_SetBits(GPIOB, GPIO_Pin_12 | GPIO_Pin_1);

    /* PB0->IRQ初始化数据中断IRQ输入引脚 */
    
    GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_0;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_OD;  
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_IN;
    GPIO_InitStruct.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_40MHz;
    
    GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* GPIO_AF_SPI2 */
    /* Connect PXx to sEE_SPI_SCK */
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource13, GPIO_AF_SPI2);

    /* Connect PXx to sEE_SPI_MISO */
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource14, GPIO_AF_SPI2); 

    /* Connect PXx to sEE_SPI_MOSI */
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource15, GPIO_AF_SPI2); 
}

/*******************************************************************************
* Function Name  : spi2_set_speed
* Description    : SPI2设置速度
* Input          : u8 Speed 
* Output         : None
* Return         : None
*******************************************************************************/
static void spi2_set_speed(INT8U SpeedSet)
{
	SPI2->CR1&=0XFFC7; 
	SPI2->CR1|=SpeedSet;
	SPI_Cmd(SPI2,ENABLE); 
} 

/**
  * @brief  根据外部SPI设备配置SPI相关参数
  * @retval None
  */
static void spi_config(void)
{
    SPI_InitTypeDef SPI_InitStruct;
 
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB | RCC_AHBPeriph_GPIOC, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
 
	SPI_InitStruct.SPI_Direction = SPI_Direction_2Lines_FullDuplex;            /* SPI设置为双线双向全双工 */
	SPI_InitStruct.SPI_Mode = SPI_Mode_Master;		                           /* SPI主机 */
    SPI_InitStruct.SPI_DataSize = SPI_DataSize_8b;		                       /* 发送接收8位帧结构 */
	SPI_InitStruct.SPI_CPOL = SPI_CPOL_Low;		                               /* 时钟悬空低 */
	SPI_InitStruct.SPI_CPHA = SPI_CPHA_1Edge;	                               /* 数据捕获于第1个时钟沿 */
	SPI_InitStruct.SPI_NSS = SPI_NSS_Soft;		                               /* NSS信号由软件控制 */
	SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;		   /* 定义波特率预分频的值:波特率预分频值为16 */
	SPI_InitStruct.SPI_FirstBit = SPI_FirstBit_MSB;	                           /* 数据传输从MSB位开始 */
	SPI_InitStruct.SPI_CRCPolynomial = 7;	                                   /* CRC值计算的多项式 */

	SPI_Init(SPI2, &SPI_InitStruct);                                           /* 根据SPI_InitStruct中指定的参数初始化外设SPIx寄存器 */     

    _spi_gpio_config();
 
    SPI_SSOutputCmd(SPI2, ENABLE);
    SPI_Cmd(SPI2, ENABLE);

    spi2_set_speed(SPI_BaudRatePrescaler_8);                                    /* spi速度为9Mhz（24L01的最大SPI时钟为10Mhz）  */
}

static INT8U spi_read_write_byte(INT8U byte)
{
    while(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET);             /* 等待发送区空 */
    /* Send byte through the SPI2 peripheral */
    SPI2->DR=byte;
    /* Wait to receive a byte */
    while(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET);
    /* Return the byte read from the SPI bus */
    return SPI2->DR;
}

/*******************************************************************
** 函数名:      write_regiter
** 函数描述:    给24L01的寄存器写值（一个字节）
** 参数:        [in] reg   : 要写的寄存器地址
**              [in] value : 给寄存器写的值
** 返回:        status 状态值
********************************************************************/
static INT8U write_regiter(INT8U reg,INT8U value)
{
	INT8U status;

	NRF24L01_CSN(0);                                                           /* CSN=0;   */
  	status  = spi_read_write_byte(reg);		                                   /* 发送寄存器地址,并读取状态值 */
	spi_read_write_byte(value);
	NRF24L01_CSN(1);                                                           /* CSN=1; */

	return status;
}

/*******************************************************************
** 函数名:      read_regiter
** 函数描述:    读24L01的寄存器值（一个字节）
** 参数:        [in] reg : 要读的寄存器地址
** 返回:        value 读出寄存器的值
********************************************************************/
static INT8U read_regiter(INT8U reg)
{
 	INT8U value;

	NRF24L01_CSN(0);                                                           /* CSN=0;   */
  	spi_read_write_byte(reg);			                                       /* 发送寄存器值(位置),并读取状态值 */
	value = spi_read_write_byte(NOP);
	NRF24L01_CSN(1);             	                                           /* CSN=1; */

	return value;
}

/*******************************************************************
** 函数名:      read_buf
** 函数描述:    读24L01的寄存器值（多个字节）
** 参数:        [in]  reg    : 寄存器地址
**              [out] packet : 读出寄存器值的存放指针
**              [in]  len    : 长度
** 返回:        status 状态值
********************************************************************/
static INT8U read_buf(INT8U reg,INT8U *packet,INT8U len)
{
	INT8U status, u8_ctr;
    
	NRF24L01_CSN(0);                     	                                   /* CSN=0 */
  	status  = spi_read_write_byte(reg);				                           /* 发送寄存器地址,并读取状态值   	   */
 	for(u8_ctr = 0; u8_ctr < len; u8_ctr++) {
    	packet[u8_ctr] = spi_read_write_byte(0XFF);		                       /* 读出数据 */
    }
	NRF24L01_CSN(1);                 		                                   /* CSN=1 */
	
  	return status;        			                                           /* 返回读到的状态值 */
}

/*******************************************************************
** 函数名:      write_buf
** 函数描述:    读24L01的寄存器值（多个字节）
** 参数:        [in]  reg    : 要写的寄存器地址
**              [out] packet : 写入寄存器值的存放指针
**              [in]  len    : 长度
** 返回:        status 状态值
********************************************************************/
static INT8U write_buf(INT8U reg, INT8U *packet, INT8U len)
{
	INT8U status, u8_ctr;
    
	NRF24L01_CSN(0);
  	status  = spi_read_write_byte(reg);			                               /* 发送寄存器值(位置),并读取状态值 */
  	for (u8_ctr = 0; u8_ctr < len; u8_ctr++) {
        //printf("%0.2x ", *packet);
    	spi_read_write_byte(*packet++); 				                       /* 写入数据 */
    }
	NRF24L01_CSN(1);
    
  	return status;          		                                           /* 返回读到的状态值 */
}	

/*******************************************************************
** 函数名:      nrf24l01_init
** 函数描述:    nrf24l01模块初始化
** 参数:        [in] rf_ch:  6:0，0000010，设置芯片工作时的信道，分别对应1~125 个道,信道间隔为1MHZ, 默认为02即2402MHz
** 参数:        [in] rf_dr:  数据传输速率，设置射频数据率为250kbps 、1Mbps 或2Mbps
** 参数:        [in] rf_pwr: 设置TX发射功率111: 7dBm,  000:-12dBm
** 返回:        true or false
********************************************************************/
BOOLEAN nrf24l01_init(INT8U rf_ch, INT8U rf_dr, INT8U rf_pwr)
{    
    spi_config();

    s_pcb.curmode = INVALID;
    nrf24l01_mode_switch(SHUTDOWN);
    
  	write_regiter(NRF_WRITE_REG+RF_CH, rf_ch);                                 /* 设置RF通道 收发必须一致，0为2.4GHz */
  	write_regiter(NRF_WRITE_REG+RF_SETUP, rf_dr | rf_pwr);                     /* 设置传输速率和发送功率 */

    write_regiter(NRF_WRITE_REG+FEATURE, 0x01);                                /* NO ACK 模式，使能命令W_TX_PAYLOAD_NOACK */   
	write_regiter(NRF_WRITE_REG+RX_PW_P0, PLOAD_WIDTH);                        /* 设置接收数据长度 */
	write_regiter(NRF_WRITE_REG+RX_PW_P1, PLOAD_WIDTH);                        /* 设置接收数据长度 */

  	write_regiter(NRF_WRITE_REG+EN_AA, 0x00);                                  /* 所有接收通道都关闭自动应答 */
  	write_regiter(NRF_WRITE_REG+EN_RXADDR, 0x03);                              /* 使能通道0和1的接收地址 */

    if (AW == 3) {
         write_regiter(NRF_WRITE_REG+SETUP_AW, AW_3);                          /* 设置地址长度为3bytes 发射方/接收方地址宽度 */
    } else if (AW == 4) {
         write_regiter(NRF_WRITE_REG+SETUP_AW, AW_4);                          /* 设置地址长度为3bytes 发射方/接收方地址宽度 */
    } else if (AW == 5) {
         write_regiter(NRF_WRITE_REG+SETUP_AW, AW_5);                          /* 设置地址长度为3bytes 发射方/接收方地址宽度 */
    }

	write_regiter(FLUSH_RX, 0xff);                                             /* 清除RX FIFO寄存器 */
	write_regiter(FLUSH_TX, 0xff);                                             /* 清除TX FIFO寄存器 */

    return nrf24l01_check();
}

/*******************************************************************
** 函数名:      nrf24l01_check
** 函数描述:    nrf24l01模块检查
** 参数:        无
** 返回:        true or false
********************************************************************/
BOOLEAN nrf24l01_check(void)
{
	INT8U i, addr[3] = {0xA5, 0xA5, 0xA5};

	//spi2_set_speed(SPI_BaudRatePrescaler_4);                                  /* spi速度为9Mhz（24L01的最大SPI时钟为10Mhz） */
	write_buf(NRF_WRITE_REG+TX_ADDR, addr, sizeof(addr));                      /* 写入3个字节的地址 */
	read_buf(TX_ADDR, addr, sizeof(addr));                                     /* 读出写入的地址  */
	
	for(i = 0; i < sizeof(addr); i++) {
        if (addr[i] != 0xA5) {
            break;                                
        }
    }

	if (i != sizeof(addr)) {
        return false;                                                          /* 检测24L01错误 */
    } else {
    	return true;		                                                   /* 检测到24L01 */
    }
} 	 

/*******************************************************************
** 函数名:      nrf24l01_mode_switch
** 函数描述:    工作模式切换
** 参数:        [in] mode: 工作模式
** 返回:        true or false
********************************************************************/
BOOLEAN nrf24l01_mode_switch(WORK_MODE_E mode)
{
    if (s_pcb.curmode == mode) {
        return true;
    }

    NRF24L01_CE(0);                                                            /* 模式切换前，先让模式回到待机模式/关断模式 */
    if (mode == SHUTDOWN) {
        write_regiter(NRF_WRITE_REG+CONFIG, DEF_COFIG);
    } else if (mode == STANDBY) {
        write_regiter(NRF_WRITE_REG+CONFIG, DEF_COFIG | PWR_UP);
    } else if (mode == RX) {
        write_regiter(FLUSH_RX, 0xff);                                         /* 清除RX FIFO寄存器 */
        write_regiter(NRF_WRITE_REG+STATUS, 0x4e);                             /* 清除RX_DR中断标志 */
        write_regiter(NRF_WRITE_REG+CONFIG, DEF_COFIG | PWR_UP | PRIM_RX);
        NRF24L01_CE(1);
    } else if (mode == IDLE_TX) {
    	write_regiter(FLUSH_TX, 0xff);                                         /* 清除TX FIFO寄存器 */
        write_regiter(NRF_WRITE_REG+STATUS, 0x2e);                             /* 清除TX_DS中断标志 */
        write_regiter(NRF_WRITE_REG+CONFIG, DEF_COFIG | PWR_UP);
        NRF24L01_CE(1);
    }

    s_pcb.curmode = mode;
    return true;
}

/*******************************************************************
** 函数名:      nrf24l01_dest_addr_set
** 函数描述:    目的地址设置
** 参数:        [in] addr: 地址指针
**              [in] len : 地址长度
** 返回:        true or false
********************************************************************/
BOOLEAN nrf24l01_dest_addr_set(INT8U* addr, INT8U len)
{
    if(len > AW) {
        return false;
    }

    memcpy(s_pcb.dest_addr, addr, len);

  	write_buf(NRF_WRITE_REG+TX_ADDR, s_pcb.dest_addr, AW);                     /* 写发送目的地址 */
    return true;
}

/*******************************************************************
** 函数名:      nrf24l01_src_addr_set
** 函数描述:    目的地址设置
** 参数:        [in] no  : 接收通道
**              [in] addr: 地址指针
**              [in] len : 地址长度
** 返回:        true or false
********************************************************************/
BOOLEAN nrf24l01_src_addr_set(INT8U no, INT8U* addr, INT8U len)
{
    INT8U reg;

    if(len > AW) {
        return false;
    }
    
    switch(no)
    {
        case 0:
            reg = NRF_WRITE_REG+RX_ADDR_P0;
            break;
        case 1:
            reg = NRF_WRITE_REG+RX_ADDR_P1;
            break;
        case 2:
            reg = NRF_WRITE_REG+RX_ADDR_P2;
            break;
        case 3:
            reg = NRF_WRITE_REG+RX_ADDR_P3;
            break;
        case 4:
            reg = NRF_WRITE_REG+RX_ADDR_P4;
            break;
        case 5:
            reg = NRF_WRITE_REG+RX_ADDR_P5;
            break;
        default:
            return false;
    }

    memcpy(s_pcb.src_addr, addr, len);
  	write_buf(reg, s_pcb.src_addr, AW);                                        /* 写本机地址 */
    return true;
}

/*******************************************************************
** 函数名:      nrf24l01_irq
** 函数描述:    发送/接收完成检查
** 参数:        无
** 返回:        true or false
********************************************************************/
BOOLEAN nrf24l01_irq(void)
{
    return GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0);                           /* IRQ主机数据输入 */
}

/*******************************************************************
** 函数名:      nrf24l01_packet_send
** 函数描述:    数据发送接口
** 参数:        [in] packet: 发送数据指针
**              [in] len   : 数据长度
** 返回:        true or false
********************************************************************/
BOOLEAN nrf24l01_packet_send(INT8U* packet, INT8U len)
{
	INT8U state;

    len = len;
    nrf24l01_mode_switch(IDLE_TX);
  	write_buf(W_TX_PAYLOAD_NO_ACK, packet, PLOAD_WIDTH);
	while (nrf24l01_irq() == 1);      									       /* 等待发送完成 */

	state = read_regiter(STATUS);                                              /* 读取状态寄存器 */	   
	write_regiter(NRF_WRITE_REG+STATUS, state);                                /* 清除TX_DS中断标志 */

	if (state&TX_OK) {                                                         /* 发送成功 */
		return true;
	}
    return false;
}

/*******************************************************************
** 函数名:      nrf24l01_packet_send
** 函数描述:    数据发送接口
** 参数:        [in] packet: 接收数据指针
**              [in] len   : 数据长度
** 返回:        接收数据长度，0为无接收到数据
********************************************************************/
INT8U nrf24l01_packet_recv(INT8U* packet, INT8U len)
{
	INT8U state;

    len = len;

	state = read_regiter(STATUS); 	 
    write_regiter(NRF_WRITE_REG+STATUS, state);                                /* 清除RX_DR中断标志 */
	if (state&RX_OK) {                                                         /* 接收到数据 */
		read_buf(RD_RX_PLOAD, packet, PLOAD_WIDTH);
		return PLOAD_WIDTH; 
	}	   
	return 0;
}

/*******************************************************************
** 函数名:      nrf24l01_get_signal
** 函数描述:    获取信号强度，1表示信号强度大约-60dBm
** 参数:        无
** 返回:        true or false
********************************************************************/
BOOLEAN nrf24l01_get_signal(void)
{
    INT8U state;
    
	state = read_regiter(CD);                                                  /* 读取载波检测寄存器 */	 

    return state&0x01;
}

#endif /* end of EN_NRF24L01 > 0 */

