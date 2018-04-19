/********************************************************************************
**
** 文件名:     eeprom.c
** 版权所有:   (c) 2013-2015 
** 文件描述:   实现内部eeprom的驱动
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**|    日期    |  作者  |  修改记录
**===============================================================================
**| 2015/08/27 | 苏友江 |  创建该文件
********************************************************************************/
#include "stm32l1xx.h"  
#include <stdio.h>

#define DATA_EEPROM_START_ADDR       0x08080100
#define DATA_EEPROM_END_ADDR         0x080807EF                                /* 最后面16bytes留给boot使用 */
#define DATA_EEPROM_SIZE             DATA_EEPROM_END_ADDR - DATA_EEPROM_START_ADDR + 1
#define DATA_EEPROM_NUM              3

typedef struct eeprom_t Eeprom_t;
struct eeprom_t {
    uint16_t offset;
    uint16_t size;
};

static uint16_t s_free = DATA_EEPROM_SIZE;
static uint8_t s_cnt = 0;
static Eeprom_t s_eeprom[DATA_EEPROM_NUM];

static uint8_t _is_eeprom_addr_available(uint32_t ee_addr)
{
	if ((ee_addr < DATA_EEPROM_START_ADDR) || (ee_addr > DATA_EEPROM_END_ADDR)) {
		return 1;
	}
	return 0;
}

uint32_t eeprom_write_byte(uint8_t num, uint16_t addr, uint8_t *buffer, uint16_t length)
{   
    uint8_t cnt = 0, ret = 0;
    uint32_t j, FLASHStatus, address;

    printf("eeprom_write start:num:%d,addr:%x,len:%d\n", num, addr, length);

    if ((num >= DATA_EEPROM_NUM) || (addr >= s_eeprom[num].size)) {
        return 1;
    }
    address = DATA_EEPROM_START_ADDR + s_eeprom[num].offset + addr;

    if (_is_eeprom_addr_available(address)) {
        return 1;
    }

    DATA_EEPROM_Unlock();	
    for (j = 0; j < length; j++) {
Retry:    
        IWDG_ReloadCounter();

        FLASHStatus = DATA_EEPROM_FastProgramByte(address, buffer[j]);
        if (FLASHStatus == FLASH_COMPLETE) {
            address++;
        } else {
            FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR
                           | FLASH_FLAG_SIZERR | FLASH_FLAG_OPTVERR);
            if (cnt++ < 5) {
                goto Retry;
            } else {
                ret = 1;
                break;
            }
        }				
    }	
    DATA_EEPROM_Lock();

    printf("test_eeprom_write_byte end\n");

    return ret;	
}

uint32_t eeprom_read_byte(uint8_t num, uint16_t addr, uint8_t *buffer, uint16_t length)
{
    uint32_t j = 0, address;

    if ((num >= DATA_EEPROM_NUM) || (addr >= s_eeprom[num].size)) {
        return 1;
    }
    address = DATA_EEPROM_START_ADDR + s_eeprom[num].offset + addr;

    if (_is_eeprom_addr_available(address)) {
        return 1;	
    }

    DATA_EEPROM_Unlock();	
    for (j = 0; j < length; j++) {
        IWDG_ReloadCounter();

        while (FLASH_GetStatus() == FLASH_BUSY);
        buffer[j] = *(__IO uint8_t*)address;
        address++;
    }
    DATA_EEPROM_Lock();
    return 0;
}

uint8_t eeprom_blank_get(uint16_t size)
{
    if ((s_free < size) || (s_cnt >= DATA_EEPROM_NUM)) {
        return 0xff;
    }

    s_eeprom[s_cnt].offset = DATA_EEPROM_SIZE - s_free;
    s_eeprom[s_cnt].size   = size;
    s_free -= size;
    
    return s_cnt++;
}


