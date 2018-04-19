/********************************************************************************
**
** 文件名:     update.c
** 版权所有:   (c) 2013-2015 
** 文件描述:   在线升级
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**|    日期    |  作者  |  修改记录
**===============================================================================
**| 2015/10/09 | 苏友江 |  创建该文件
********************************************************************************/
#include "bsp.h"
#include "sys_includes.h"
#if EN_UPDATE > 0 

#if DBG_GENERAL > 0
#define SYS_DEBUG          OS_DEBUG
#else
#define SYS_DEBUG(...)     do{}while(0)
#endif
/*
********************************************************************************
* 注: 本升级程序还未验证过，机制过于简单，有以下几个不足:
* 1.终端重启后，没有保存之前升级状态，只能重新升级
* 2.flash是按照页来擦除，在写入升级包数据的流程是:一、读取页数据；二、擦除页数据
* 三、修改缓存中的页数据；四、把修改的缓存按页写入；因此如果在写入页出现问题，将
* 有可能导致其它已经写好的升级包遭到破坏，但是MAP标识位还认为该包数据时有效的
* 这种情况导致的后果是，接收所有包后，在crc校验会出错，如果碰巧crc正确了，这样会
* 导致程序运行不可控的错误。
********************************************************************************
*/


/*
********************************************************************************
* 宏定义
********************************************************************************
*/
#define APP_START_ADDR          (0x08002000)   /* app0开始存放位置, 0x2000是给boot使用8k */
#define APP_END_ADDR	        (0x08020000)   /* flash的边缘 */
#define APP_MAX_SIZE	        (0xf000)       /* 定义APP的大小,60k */
#define APP_PAGE_SIZE	        (0x100)        /* flash的页大小,256bytes */

#define UPDATE_PKG_MAX_SIZE     (160)          /* 定义升级包最大不能超过160bytes */
#define UPDATE_PKG_MAP_MAX_SIZE (128)          /* 定义接收包映射表的大小,最大可接收包数:128*8 */

#define EEPROM_UPDATE_FLAG_ADDR (0x080807F0)   /* 和boot共享此参数，因此地址必须和boot一致 */

/*
********************************************************************************
* 结构定义
********************************************************************************
*/
typedef struct {
    INT16U version;          /* 要升级程序的版本号 */
    INT32U filesize;         /* 升级文件大小 */
    INT16U filecrc;          /* 文件校验 */
    INT8U  pkgsize;          /* 每包大小 */
} Update_info_t;

typedef struct {
    INT16U total_pkg;        /* 升级程序的总包数 */
    INT16U total_page;       /* 升级程序占的总页数，因为flash按页来读写 */
    Update_info_t info;      /* 上位机下发的升级信息 */
                             /* 每个位用来标识升级包的接收情况 */
    INT8U  pkgmap[UPDATE_PKG_MAP_MAX_SIZE]; 
} Priv_t;

/*
********************************************************************************
* 静态变量
********************************************************************************
*/
static Priv_t s_priv;
static INT8U  s_page_buf[APP_PAGE_SIZE] = {0}; 

/*******************************************************************
** 函数名:      _save_update_flag
** 函数描述:    保存update标志，1表示需要升级
** 参数:        [in] flag
** 返回:        none
********************************************************************/
static void _save_update_flag(INT8U flag)
{
    INT8U cnt = 0;

    DATA_EEPROM_Unlock();	
Retry:    
    IWDG_ReloadCounter();

    if (DATA_EEPROM_FastProgramByte(EEPROM_UPDATE_FLAG_ADDR, flag) != FLASH_COMPLETE) {
        FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR
                       | FLASH_FLAG_SIZERR | FLASH_FLAG_OPTVERR);
        if (cnt++ < 5) {
            goto Retry;
        }
    }				
    DATA_EEPROM_Lock();
}

/*******************************************************************
** 函数名:      _reset_update_para
** 函数描述:    复位升级参数
** 参数:        none
** 返回:        none
********************************************************************/
static void _reset_update_para(void)
{
    memset(&s_priv, 0, sizeof(s_priv));
}

/*******************************************************************
** 函数名:      _get_bit_in_array
** 函数描述:    根据索引查找升级package map表中对应的位
** 参数:        [in] idx
** 返回:        true or false
********************************************************************/
static BOOLEAN _get_bit_in_array(INT16U idx)
{
	INT8U ret = 0;
	INT8U arrayIndex = idx / 8;
	INT8U byteOffset = idx % 8;

	ret = s_priv.pkgmap[arrayIndex] & (0x1 << byteOffset);
	return (ret != 0)?(TRUE):(FALSE);
}

/*******************************************************************
** 函数名:      _set_bit_in_array
** 函数描述:    根据索引号置位package map 中指定的位
** 参数:        [in] idx
** 返回:        true or false
********************************************************************/
static BOOLEAN _set_bit_in_array(INT16U idx)
{
	INT8U arrayIndex = idx / 8;
	INT8U byteOffset = idx % 8;

    if (arrayIndex < UPDATE_PKG_MAP_MAX_SIZE) {
    	s_priv.pkgmap[arrayIndex] |= (0x1 << byteOffset);
        return TRUE;
    } else {
        return FALSE;
    }
}

#if 0
/*******************************************************************
** 函数名:      _reset_bit_in_array
** 函数描述:    根据索引号复位package map 中指定的位
** 参数:        [in] idx
** 返回:        true or false
********************************************************************/
static BOOLEAN _reset_bit_in_array(INT16U idx)
{
	INT8U arrayIndex = idx / 8;
	INT8U byteOffset = idx % 8;

    if (arrayIndex < UPDATE_PKG_MAP_MAX_SIZE) {
    	s_priv.pkgmap[arrayIndex] &= ~(0x1 << byteOffset);
        return TRUE;
    } else {
        return FALSE;
    }
}
#endif /* if 0. 2016-1-31 1:10:07 suyoujiang */

/*******************************************************************
** 函数名:      _write_page
** 函数描述:    往flash中写入一个页
** 参数:        [in] offset
**              [in] pdata
**              [in] len
** 返回:        true or false
********************************************************************/
static BOOLEAN _write_page(INT32U offset, INT8U *pData, INT32U len)
{
    BOOLEAN ret = TRUE;
	INT32U statrAddr = 0;
	INT32U off = 0;
	INT32U data = 0;

    if (((offset + len) > APP_MAX_SIZE) || (len != APP_PAGE_SIZE)) {
        return FALSE;
    }

	statrAddr = APP_START_ADDR + offset;
	bsp_watchdog_feed();
	FLASH_Unlock();
  
    if (FLASH_ErasePage(statrAddr) != FLASH_COMPLETE) {
    	ret = FALSE;
    } else {
    	off = 0;

    	while (off + 3 < APP_PAGE_SIZE) {
    		data = *(INT32U *)(pData + off);                                   /* 注意必须保证传入的pData是4字节对齐，否则写入的数据将会出错 */
    		if (FLASH_FastProgramWord(statrAddr + off, data) == FLASH_COMPLETE) {
    			off = off + sizeof(INT32U);
    		} else { 
    			ret = FALSE;
    			break;
    		}
    	}
    }

	FLASH_Lock();
	
	return ret;
}

/*******************************************************************
** 函数名:      _read_page
** 函数描述:    读取flash数据
** 参数:        [in] offset
**              [in] pdata
**              [in] len
** 返回:        true or false
********************************************************************/
static BOOLEAN _read_page(INT32U offset, INT8U *pdata, INT32U len)
{
	INT32U statrAddr;

    if ((offset + len) > APP_MAX_SIZE) {
        return FALSE;
    }

	statrAddr = APP_START_ADDR + offset;
	bsp_watchdog_feed();
	
	while (len-- > 0) {
        *pdata++ = *(__IO uint8_t *)(statrAddr++);
	}
	
	return TRUE;
}

static BOOLEAN _write_update_pgk(INT16U pkg, INT8U *pData, INT32U len)
{
    BOOLEAN ret = TRUE;
	INT16U targetPage = pkg * s_priv.info.pkgsize / APP_PAGE_SIZE;
	INT8U  pageOff    = pkg * s_priv.info.pkgsize - APP_PAGE_SIZE * targetPage;
	INT32U left = 0;
	
	if (_read_page(targetPage * APP_PAGE_SIZE, s_page_buf, APP_PAGE_SIZE) == FALSE) {
        ret = FALSE;
    }
		
	if (pageOff + len < APP_PAGE_SIZE) {
		memcpy(s_page_buf + pageOff, pData, len);
		if (_write_page(targetPage * APP_PAGE_SIZE, s_page_buf, APP_PAGE_SIZE) == FALSE) {
            ret = FALSE;
        }
	} else {
		memcpy(s_page_buf + pageOff, pData, APP_PAGE_SIZE - pageOff);
		if (_write_page(targetPage * APP_PAGE_SIZE, s_page_buf, APP_PAGE_SIZE) == FALSE) {
            ret = FALSE;
		}
		left = len + pageOff - APP_PAGE_SIZE;

		if (left > 0) {
            targetPage++;
			if (_read_page(targetPage * APP_PAGE_SIZE, s_page_buf, APP_PAGE_SIZE) == FALSE) {
                ret = FALSE;
    		}
			memcpy(s_page_buf, pData + APP_PAGE_SIZE - pageOff, left);
			if (_write_page(targetPage * APP_PAGE_SIZE, s_page_buf, APP_PAGE_SIZE) == FALSE) {
                ret = FALSE;
    		}
		}
	}
	return ret;
}

/*******************************************************************
** 函数名:      _check_update
** 函数描述:     检测是否可以升级
** 参数:        none
** 返回:        none
********************************************************************/
static void _check_update(void)
{
	INT16U i, len;
    static INT16U crc16 = 0xffff;
    INT32U left;
	
	for (i = 0; i < s_priv.total_pkg; i++) {
		if (_get_bit_in_array(i) != TRUE) {                                    /* 升级包未接收完 */
            return;
		}
	}

    left = s_priv.info.filesize;
    for (i = 0; i < s_priv.total_page; i++) {
    	_read_page(i * APP_PAGE_SIZE, s_page_buf, APP_PAGE_SIZE);

    	if (left >= APP_PAGE_SIZE) {
    		len = APP_PAGE_SIZE;
        } else {
    		len = left;
        }
    	
    	if (i == 0) {
    		ut_crc16((INT8U *)&crc16, s_page_buf, len);
        } else {
    		ut_crc16_separate((INT8U *)&crc16, s_page_buf, len);
        }

    	left -= len;
    }

    if (crc16 == s_priv.info.filecrc) {
		_save_update_flag(1);
    	__set_FAULTMASK(1);
    	NVIC_SystemReset();
    } else {
        SYS_DEBUG("<---update file crc fault!!!>\n");
        crc16 = 0xffff;
        _reset_update_para();
    }
}

/*******************************************************************
** 函数名:      _handle_0x8210
** 函数描述:    升级信息下发
** 参数:        [in] packet:
** 返回:        none
********************************************************************/
static void _handle_0x8210(Comm_pkt_recv_t *packet)
{
    Stream_t rstrm, wstrm;
    INT8U result = 0, sendbuf[3];
    INT16U cur_ver;
    Update_info_t info;
    Comm_pkt_send_t send_pkt;
    
    if (10 != packet->len) {
        result = 3;                                                            /* 数据长度有误，数据长度为10byte，参见协议 */
        goto EXIT;
    }

    stream_init(&rstrm, packet->pdata, packet->len);
    info.version = stream_read_half_word(&rstrm);

    if (stream_read_byte(&rstrm) == 0) {                                       /* 不是强制升级，需要判断版本号 */
        cur_ver = atoi(SOFTWARE_VERSION_STR);
        if (info.version < cur_ver) {
            result = 2;                                                        /* 升级版本号小于当前版本号 */
            goto EXIT;        
        } else if (info.version == cur_ver) {
            result = 1;                                                        /* 升级版本号等于当前版本号 */
            goto EXIT;   
        }  
    }

    info.filesize = stream_read_long(&rstrm);
    info.filecrc  = stream_read_half_word(&rstrm);

    info.pkgsize = stream_read_byte(&rstrm);
    if (info.pkgsize > UPDATE_PKG_MAX_SIZE) {
        result = 3;
        goto EXIT;
    } 

    if (memcmp(&s_priv.info, &info, sizeof(info)) != 0) {                      /* 如果下发的升级信息和之前存储的不一样 */
        memcpy(&s_priv.info, &info, sizeof(info));
        memset(s_priv.pkgmap, 0, UPDATE_PKG_MAP_MAX_SIZE);                     /* 则清空pkgmap标识位，放弃之前接收过的数据 */

    	s_priv.total_pkg = (s_priv.info.filesize % s_priv.info.pkgsize == 0) ? 
    		(s_priv.info.filesize / s_priv.info.pkgsize) : (s_priv.info.filesize / s_priv.info.pkgsize + 1);
    	s_priv.total_page = (s_priv.info.filesize % APP_PAGE_SIZE == 0) ? 
    		(s_priv.info.filesize / APP_PAGE_SIZE) : (s_priv.info.filesize / APP_PAGE_SIZE + 1);
    }
    
EXIT:    
    if (!packet->ack) {                                                        /* 无需应答 */
        return;
    }

    stream_init(&wstrm, sendbuf, sizeof(sendbuf));
    stream_write_half_word(&wstrm, packet->flowseq);
    stream_write_byte(&wstrm, result);

    send_pkt.len   = stream_get_len(&wstrm);
    send_pkt.pdata = sendbuf;
    send_pkt.msgid = 0x0210;
    comm_send_dirsend(&send_pkt);
}

/*******************************************************************
** 函数名:      _handle_0x8211
** 函数描述:    升级数据包下发
** 参数:        [in] packet:
** 返回:        none
********************************************************************/
static void _handle_0x8211(Comm_pkt_recv_t *packet)
{
    Stream_t rstrm, wstrm;
    INT8U result = 0, sendbuf[3];
    INT16U pkgidx, crc, calc_crc, calc_pkgsize = 0;
    Comm_pkt_send_t send_pkt;

    if (packet->len < 10) {
        result = 1;                                                            /* 数据长度有误 */
        goto EXIT;
    }

    stream_init(&rstrm, packet->pdata, packet->len);
    
    if (stream_read_long(&rstrm) != s_priv.info.filesize) {
        result = 1;
        goto EXIT;
    }

    if (stream_read_half_word(&rstrm) != s_priv.info.filecrc) {
        result = 1;
        goto EXIT;
    }

    pkgidx = stream_read_half_word(&rstrm);
    crc    = stream_read_half_word(&rstrm);

    if (pkgidx < s_priv.total_pkg - 1) {
    	calc_pkgsize = s_priv.info.pkgsize;
    } else if (pkgidx == s_priv.total_pkg - 1) {                               /* 最后一包 */
    	calc_pkgsize = s_priv.info.filesize - pkgidx * s_priv.info.pkgsize;
    }

    if ((pkgidx >= s_priv.total_pkg) || (stream_get_left_len(&rstrm) != calc_pkgsize)) {
        result = 1;
        goto EXIT;
    }

    calc_crc = 0;                                                              /* 必须清0，否则会传入不确定值到校验函数中 */
    ut_crc16((INT8U *)&calc_crc, stream_get_pointer(&rstrm), stream_get_left_len(&rstrm));
    if (calc_crc != crc) {
    	SYS_DEBUG("< update crc error!!!>\n");
        result = 1;
        goto EXIT;
    }

    if (_get_bit_in_array(pkgidx) == FALSE) {
        if (_write_update_pgk(pkgidx, stream_get_pointer(&rstrm), stream_get_left_len(&rstrm))) {
        	_set_bit_in_array(pkgidx);
        } else {
        	SYS_DEBUG("< _write_update_pgk fail>\n");
        }
    	_check_update();
    }

EXIT:    
    if (!packet->ack) {                                                        /* 无需应答 */
        return;
    }

    stream_init(&wstrm, sendbuf, sizeof(sendbuf));
    stream_write_half_word(&wstrm, packet->flowseq);
    stream_write_byte(&wstrm, result);

    send_pkt.len   = stream_get_len(&wstrm);
    send_pkt.pdata = sendbuf;
    send_pkt.msgid = 0x0211;
    comm_send_dirsend(&send_pkt);
}

/*******************************************************************
** 函数名:      _handle_0x8212
** 函数描述:    升级状态查询
** 参数:        [in] packet:
** 返回:        none
********************************************************************/
static void _handle_0x8212(Comm_pkt_recv_t *packet)
{
    #define SENDBUF_SIZE (2+4+2+UPDATE_PKG_MAP_MAX_SIZE)
    Stream_t wstrm;
    INT8U *psendbuf;
    Comm_pkt_send_t send_pkt;

    if (!packet->ack) {                                                        /* 无需应答 */
        return;
    }

    psendbuf = (INT8U*)mem_malloc(SENDBUF_SIZE);
    if (psendbuf == NULL) {
        return;
    }
    stream_init(&wstrm, psendbuf, SENDBUF_SIZE);
    stream_write_half_word(&wstrm, s_priv.info.version);
    stream_write_long(&wstrm, s_priv.info.filesize);
    stream_write_half_word(&wstrm, s_priv.info.filecrc);
    stream_write_data(&wstrm, s_priv.pkgmap, UPDATE_PKG_MAP_MAX_SIZE);

    send_pkt.len   = stream_get_len(&wstrm);
    send_pkt.pdata = psendbuf;
    send_pkt.msgid = 0x0212;
    comm_send_dirsend(&send_pkt);
    mem_free(psendbuf);
}

/*
********************************************************************************
* 注册回调函数
********************************************************************************
*/
static const FUNCENTRY_COMM_T s_functionentry[] = {
        0x8210, _handle_0x8210       /* 升级信息下发 */
       ,0x8211, _handle_0x8211       /* 升级数据下发 */
       ,0x8212, _handle_0x8212       /* 升级状态查询 */
};
static const INT8U s_funnum = sizeof(s_functionentry) / sizeof(s_functionentry[0]);

/*******************************************************************
** 函数名:      update_init
** 函数描述:    远程升级初始化
** 参数:        无
** 返回:        无
********************************************************************/
void update_init(void)
{
	INT8U i;

    _reset_update_para();

    for (i = 0; i < s_funnum; i++) {
        comm_recv_register(s_functionentry[i].index, s_functionentry[i].entryproc);
    }
}
#endif /* end of EN_UPDATE */

