/********************************************************************************
**
** 文件名:     i2c_ds1338.c
** 版权所有:   (c) 2013-2015 
** 文件描述:   时钟芯片DS1338驱动,本驱动函数兼容DS1307、DS1338、DS1338Z芯片
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**|    日期    |  作者  |  修改记录
**===============================================================================
**| 2015/7/6 | 苏友江 |  创建该文件
********************************************************************************/
#include "stm32l1xx.h"  
#include "i2c_ds1338.h"


/******************************************************************************
                                    定义变量
******************************************************************************/

Time_Typedef TimeValue;                 /* 定义时间缓存指针 */

#define EE_SCL_SET() GPIOB->BSRRL |= GPIO_Pin_10;
#define EE_SCL_CLR() GPIOB->ODR   &= ~GPIO_Pin_10;
#define EE_SDA_SET() GPIOB->BSRRL |= GPIO_Pin_11;
#define EE_SDA_CLR() GPIOB->ODR   &= ~GPIO_Pin_11;
#define EE_SDA_RD() GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_11)

#define EE_DEALY() delay_us(1);
static void delay_us(uint32_t us)
{
	uint32_t i;
	for (i = 0; i < us*3+us/2; i++);
}

static void IIC_Start(void)                                                    /* 起始信号 */
{
	EE_DEALY();
    EE_SDA_SET();
	EE_DEALY();
    EE_SCL_SET();
    EE_DEALY();
    EE_SDA_CLR();
    EE_DEALY();
    EE_SCL_CLR();	  
}

static void IIC_Stop(void)                                                     /* 停止信号 */
{
	EE_DEALY();	
    EE_SDA_CLR();
	EE_DEALY();	
    EE_SCL_SET();
    EE_DEALY();	
    EE_SDA_SET();
	EE_DEALY();	 
}

static uint8_t EE_waitack(void)                                                /* 等待应答信号 */
{
	uint16_t i=0;
	uint8_t status=0;
	static uint8_t ccc=0;
	
	EE_SDA_SET();
	EE_DEALY();
    EE_SCL_SET();

    while(EE_SDA_RD()==1)
	{
		if(i++>500)
		{
			status =1;
			ccc++;
			break;
		}	
	}
	if((i<500)||(EE_SDA_RD()==0))
	{
		status=0;
		ccc=0;
	}
	EE_DEALY();	
    EE_SCL_CLR();
	return status;	
}

static void EE_noack(void)                                                   /* 非应答信号 */
{
	  EE_SDA_SET();	                                                           /* sda=1 */
	  EE_DEALY();			                                                   /* 延时5us */
	  EE_SCL_SET();	                                                           /* scl=1 */
	  EE_DEALY();			                                                   /* 延时5us */
	  EE_SCL_CLR();	                                                           /* scl=0 */
	  EE_DEALY();			                                                   /* 延时5us */
	  EE_SDA_CLR();	                                                           /* sda=0 */
	  EE_DEALY();
}

static void EE_ack(void)	                                                   /* 应答信号 */
{ 
	  EE_SDA_CLR();	                                                           /* sda=0 */
	  EE_DEALY();			                                                   /* 延时5us */
	  EE_SCL_SET();	                                                           /* scl=1 */
	  EE_DEALY(); 			                                                   /* 延时5us */
	  EE_SCL_CLR();	                                                           /* scl=0 */
	  EE_DEALY();				                                               /* 延时5us */
	  EE_SDA_SET();	                                                           /* sda=1 */
	  EE_DEALY();
}

static void IIC_Ack(uint8_t ack)
{
    if (ack == 0) {
        EE_ack();
    } else {
        EE_noack();
    }
}

static uint8_t IIC_Send_Byte(uint8_t input)	                               /* 写一个字节数据 */
{
    unsigned char i;
    
    for(i=0;i<8;i++)
    {
        if((input&0x80)>0)
        {
            EE_DEALY();
            EE_SDA_SET();
            EE_DEALY();
            EE_SCL_SET();
            EE_DEALY();
            EE_SCL_CLR();
        }
        else
        {
            EE_DEALY();
            EE_SDA_CLR();
            EE_DEALY();
            EE_SCL_SET();
            EE_DEALY();
            EE_SCL_CLR();
        }
        input=input<<1;
    }	

    return EE_waitack();
}

static uint8_t IIC_Read_Byte(void)		                                   /* 读一个字节数据 */
{
    unsigned char num=0xff,mm=0x01,uu=0xfe;
    unsigned char j;
	                                                                           /* EE_SDA_SET(); */
    for(j=0;j<8;j++)
    {
        EE_DEALY();
        EE_SCL_SET();
        EE_DEALY();
        num<<=1;
        if(EE_SDA_RD()==0)
            num=(num&uu);
        else
        {
            num=(num|mm);
        }		
        EE_SCL_CLR();
        EE_DEALY();
    }

    return(num);
}

static void ds1338_i2c_init(void) 
{                 
    GPIO_InitTypeDef GPIO_InitStructure;	
    /*SDA SCL GPIO clock enable */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB , ENABLE);

    /*!< Configure AT24LCxxx_I2C pins: SCL SDA */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_10|GPIO_Pin_11;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;  
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);  
} 
        
/*******************************************************************
** 函数名:      ds1338_write_byte
** 函数描述:    寄存器写入一个字节数据
** 参数:        [in] reg_addr：要操作寄存器地址
**              [in] dat：要写入的数据
** 返回:        none
********************************************************************/
void ds1338_write_byte(uint8_t reg_addr, uint8_t dat)
{
	IIC_Start();
	if(!(IIC_Send_Byte(DS1307_Write)))                                        /* 发送写命令并检查应答位 */
	{
		IIC_Send_Byte(reg_addr);
		IIC_Send_Byte(dat);
	}
	IIC_Stop();
}

/*******************************************************************
** 函数名:      ds1338_read_byte
** 函数描述:    寄存器读取一个字节数据
** 参数:        [in] reg_addr：要操作寄存器地址
** 返回:        读取到的数值
********************************************************************/
uint8_t ds1338_read_byte(uint8_t reg_addr)
{
	uint8_t rcv = 0;

	IIC_Start();
	if(!(IIC_Send_Byte(DS1307_Write)))                                         /* 发送写命令并检查应答位 */
	{
        IIC_Send_Byte(reg_addr);                                               /* 发送要操作的寄存器地址 */
        IIC_Start();                                                           /* 重启总线 */
        IIC_Send_Byte(DS1307_Read);                                            /* 发送读取命令 */
        rcv = IIC_Read_Byte();
        IIC_Ack(0x01);                                                         /* 发送非应答信号 */
	}
	IIC_Stop();
	return rcv;
}

/*******************************************************************
** 函数名:      ds1338_operate_register
** 函数描述:    对时间日历寄存器操作，写入数据或者读取数据
**              连续写入n字节或者连续读取n字节数据
** 参数:        [in] reg_addr：要操作寄存器起始地址
**              [in] pbuf:   写入数据缓存
**              [in] num：写入数据数量
**              [in] mode：操作模式。0：写入数据操作。1：读取数据操作
** 返回:        none
********************************************************************/
void ds1338_operate_register(uint8_t reg_addr,uint8_t *pbuf,uint8_t num,uint8_t mode)
{
	uint8_t i;
	if(mode)	                                                               /* 读取数据 */
	{
		IIC_Start();
		if(!(IIC_Send_Byte(DS1307_Write)))	                                   /* 发送写命令并检查应答位 */
		{
			IIC_Send_Byte(reg_addr);	                                       /* 定位起始寄存器地址 */
			IIC_Start();	                                                   /* 重启总线 */
			IIC_Send_Byte(DS1307_Read);	                                       /* 发送读取命令 */
			for(i = 0;i < num;i++)
			{
				*pbuf = IIC_Read_Byte();	                                   /* 读取数据 */
				if(i == (num - 1))	IIC_Ack(0x01);	                           /* 发送非应答信号 */
				else IIC_Ack(0x00);	                                           /* 发送应答信号 */
				pbuf++;
			}
		}
		IIC_Stop();	
	}
	else	                                                                   /* 写入数据 */
	{		 	
		IIC_Start();
		if(!(IIC_Send_Byte(DS1307_Write)))	                                   /* 发送写命令并检查应答位 */
		{
			IIC_Send_Byte(reg_addr);	                                           /* 定位起始寄存器地址 */
			for(i = 0;i < num;i++)
			{
				IIC_Send_Byte(*pbuf);	                                       /* 写入数据 */
				pbuf++;
			}
		}
		IIC_Stop();
	}
}

/*******************************************************************
** 函数名:      ds1338_readWrite_time
** 函数描述:    读取或者写入时间信息
** 参数:        [in] mode：操作模式。0：写入数据操作。1：读取数据操作
** 返回:        none
********************************************************************/
void ds1338_readWrite_time(uint8_t mode)
{
	uint8_t Time_Register[8];	                                               /* 定义时间缓存 */
	
	if(mode)	                                                               /* 读取时间信息 */
	{
		ds1338_operate_register(Address_second,Time_Register,7,1);	           /* 从秒地址（0x00）开始读取时间日历数据 */
		
	    /******将数据复制到时间结构体中，方便后面程序调用******/
		TimeValue.second = Time_Register[0] & Shield_secondBit;	               /* 秒数据 */
		TimeValue.minute = Time_Register[1] & Shield_minuteBit;	               /* 分钟数据 */
		TimeValue.hour   = Time_Register[2] & Shield_hourBit;	               /* 小时数据 */
		TimeValue.week   = Time_Register[3] & Shield_weekBit;	               /* 星期数据 */
		TimeValue.date   = Time_Register[4] & Shield_dateBit;	               /* 日数据 */
		TimeValue.month  = Time_Register[5] & Shield_monthBit;	               /* 月数据 */
		TimeValue.year   = Time_Register[6];	                               /* 年数据 */
	}
	else
	{
	    /******从时间结构体中复制数据进来******/
		Time_Register[0] = TimeValue.second | Control_Chip_Run;	               /* 秒，启动芯片 */
		Time_Register[1] = TimeValue.minute;	                               /* 分钟 */
		Time_Register[2] = TimeValue.hour | Hour_Mode24;	                   /* 小时，24小时制 */
		Time_Register[3] = TimeValue.week;	                                   /* 星期 */
		Time_Register[4] = TimeValue.date;	                                   /* 日		 */
		Time_Register[5] = TimeValue.month;	                                   /* 月 */
		Time_Register[6] = TimeValue.year;	                                   /* 年 */

    #if Chip_Type==1                                                           /* 如果定义了，则使用的是DS1307芯片 */
		Time_Register[7] = CLKOUT_f32768;	                                   /* 频率输出控制 */
    #else                                                                      /* 没定义，则使用的是DS1338或者DS1338Z芯片 */
		Time_Register[7] = CLKOUT_f32768 | OSF_Enable;	                       /* 频率输出控制 */
    #endif

		ds1338_operate_register(Address_second,Time_Register,8,0);	           /* 从秒地址（0x00）开始写入时间日历数据 */
    	ds1338_write_byte(RAM_Address55, CHECK_FLAG);                          /* 向最后一个RAM地址写入识别值 */
	}
}

/*******************************************************************
** 函数名:      ds1338_check
** 函数描述:    检测函数 在DS1307芯片的RAM的最后一个地址写入一个数据并读出来判断
**              与上次写入的值相等，不是第一次上电，否则则初始化时间
** 参数:        none
** 返回:        0：设备正常并不是第一次上电, 1：设备错误或者已损坏
********************************************************************/
uint8_t ds1338_check(void)
{
	if(ds1338_read_byte(RAM_Address55) == CHECK_FLAG)    return 0;              /* 设备正常，不是第一次上电 */
	else    return 1;
}

#if 0/* 由于flash空间不够，暂时先屏蔽这两个ram读写函数 */
/*******************************************************************
** 函数名:      ds1338_ram_write_data
** 函数描述:    内置的RAM写数据操作
** 参数:        [in] pbuf：写数据存放区
**              [in] WRadd：读写起始地址，范围在RAM_Address0 ~ RAM_Address55之间，最后一位地址有其他用途
**              [in] num：读写字节数据的数量，范围在1 ~ 55之间
** 返回:        none
********************************************************************/
void ds1338_ram_write_data(uint8_t* pbuf, uint8_t WRadd, uint8_t num)
{
	uint8_t i;
	uint8_t ADDremain;                                                           /* 写入数据组数 */
    
	/******判断写入数据的起始地址范围******/
	if(WRadd >= RAM_Address55)  return;                                        /* 最后一个RAM单元操作，直接退出 */

	/******判断发送数据的组数目******/
	if((WRadd + num) >= (RAM_Address55 - 1))    ADDremain = RAM_Address55 - 1 - WRadd;  /* 超出范围，写入余下的空间 */
	else    ADDremain = num;                                                   /* 没超出空间，直接写完 */

	IIC_Start();
	if(!(IIC_Send_Byte(DS1307_Write)))                                         /* 发送写命令并检查应答信号 */
	{
		IIC_Send_Byte(WRadd);                                                  /* 发送写入数据首地址 */
		for(i = 0;i < ADDremain;i++)
		{
			IIC_Send_Byte(pbuf[i]);                                           /* 写入数据 */
		}
	}
	IIC_Stop();
}

/*******************************************************************
** 函数名:      ds1338_ram_read_data
** 函数描述:    内置的RAM读数据操作
** 参数:        [in] pbuf：读数据存放区
**              [in] WRadd：读写起始地址，范围在RAM_Address0 ~ RAM_Address55之间，最后一位地址有其他用途
**              [in] num：读写字节数据的数量，范围在1 ~ 55之间
** 返回:        none
********************************************************************/
void ds1338_ram_read_data(uint8_t* pbuf, uint8_t WRadd, uint8_t num)
{
	uint8_t i;
	uint8_t ADDremain;

	/******判断读取数据的起始地址范围******/
	if(WRadd >= RAM_Address55)  return;                                        /* 最后一个RAM单元操作，直接退出 */

	/******最后一个地址被用作检测DS1307来用，所以不读最后一个地址数据******/
	if((WRadd + num) >= RAM_Address55)  ADDremain = RAM_Address55 - 1 - WRadd; /* 超出范围了，读取起始地址到倒数第二个地址空间的数据 */
	else    ADDremain = num;                                                   /* 没超出地址范围，全部读取完 */

	IIC_Start();
	if(!(IIC_Send_Byte(DS1307_Write)))                                         /* 发送写命令并检查应答信号 */
	{
		IIC_Send_Byte(WRadd);                                                  /* 发送读取数据开始寄存器地址 */
		IIC_Start();
		if(!(IIC_Send_Byte(DS1307_Read)))                                      /* 发送读取命令并检查应答信号 */
		{
			for(i = 0;i < ADDremain;i++)
			{
				pbuf[i] = IIC_Read_Byte();                                    /* 开始接收num组数据 */
				if(i == (ADDremain - 1))    IIC_Ack(0x01);                     /* 读取完最后一组数据，发送非应答信号 */
				else    IIC_Ack(0x00);                                         /* 发送应答信号 */
			}
		}
	}
	IIC_Stop();
}
#endif /* if 0. 2015-7-9 09:06:07 syj */

/*******************************************************************
** 函数名:      ds1338_init
** 函数描述:    时间日历初始化
** 参数:        [in] TimeVAL：RTC芯片寄存器值指针
** 返回:        none
********************************************************************/
void ds1338_init(void)
{	
    ds1338_i2c_init();

}

