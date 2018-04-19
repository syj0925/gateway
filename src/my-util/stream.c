/********************************************************************************
**
** 文件名:     stream.c
** 版权所有:   (c) 2013-2014 厦门市智联信通物联网科技有限公司
** 文件描述:   实现数据流管理功能
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**|    日期    |  作者  |  修改记录
**===============================================================================
**| 2014/12/18 | 苏友江 |  创建该文件
********************************************************************************/
#include "sys_typedef.h"
#include "stream.h"
#include <stdio.h>
#include <stdarg.h>
/*
********************************************************************************
* 参数配置
********************************************************************************
*/
#ifndef	 CR
#define  CR                      0x0D
#endif

#ifndef  LF
#define  LF                      0x0A
#endif

/*******************************************************************
** 函数名:     stream_init
** 函数描述:   初始化数据流
** 参数:       [in] pstrm:  数据流
**             [in] pbuf:   数据流所管理内存地址
**             [in] buflen: 数据流所管理内存字节数
** 返回:       成功或失败
********************************************************************/
BOOLEAN stream_init(Stream_t *pstrm, INT8U *pbuf, INT16U buflen)
{
    if (pstrm == 0) return FALSE;
	
    pstrm->len      = 0;
    pstrm->maxlen   = buflen;
    pstrm->current  = pbuf;
    pstrm->start    = pbuf;
    return TRUE;
}

/*******************************************************************
** 函数名:     stream_get_len
** 函数描述:   获取数据流中已用字节数
** 参数:       [in] pstrm: 数据流
** 返回:       数据流已用字节数
********************************************************************/
INT16U stream_get_len(Stream_t *pstrm)
{
    return (pstrm->len);
}

/*******************************************************************
** 函数名:     stream_get_left_len
** 函数描述:   获取数据流中剩余的可用字节数
** 参数:       [in] pstrm: 数据流
** 返回:       数据流剩余的可用字节数
********************************************************************/
INT16U stream_get_left_len(Stream_t *pstrm)
{
    if (pstrm->maxlen >= pstrm->len) {
        return (pstrm->maxlen - pstrm->len);
    } else {
        return 0;
    }
}

/*******************************************************************
** 函数名:     stream_get_maxlen
** 函数描述:   获取数据流缓存总长度
** 参数:       [in] pstrm: 数据流
** 返回:       数据流的最大长度
********************************************************************/
INT16U stream_get_maxlen(Stream_t *pstrm)
{
    return (pstrm->maxlen);
}

/*******************************************************************
** 函数名:     stream_get_pointer
** 函数描述:   获取数据流当前读/写指针
** 参数:       [in] pstrm: 数据流
** 返回:       当前读/写指针
********************************************************************/
INT8U *stream_get_pointer(Stream_t *pstrm)
{
    return (pstrm->current);
}

/*******************************************************************
** 函数名:     stream_get_start_pointer
** 函数描述:   获取数据流所管理内存的地址
** 参数:       [in] pstrm: 数据流
** 返回:       所管理内存地址
********************************************************************/
INT8U *stream_get_start_pointer(Stream_t *pstrm)
{
    return (pstrm->start);
}

/*******************************************************************
** 函数名:     stream_move_pointer
** 函数描述:   移动数据流中读/写指针
** 参数:       [in] pstrm: 数据流
**             [in] len:   移动字节数
** 返回:       无
********************************************************************/
void stream_move_pointer(Stream_t *pstrm, INT16U len)
{
    if (pstrm != 0) {
        if ((pstrm->len + len) <= pstrm->maxlen) {
            pstrm->len     += len;
            pstrm->current += len;
        } else {
            pstrm->len = pstrm->maxlen;
        }
    }
}

/*******************************************************************
** 函数名:     stream_write_byte
** 函数描述:   往数据流中写入一个字节数据
** 参数:       [in] pstrm:     数据流
**             [in] writebyte: 写入的数据
** 返回:       无
********************************************************************/
void stream_write_byte(Stream_t *pstrm, INT8U writebyte)
{
    if (pstrm != 0) {
        if (pstrm->len < pstrm->maxlen) {
            *pstrm->current++ = writebyte;
            pstrm->len++;
        }
    }
}

/*******************************************************************
** 函数名:     stream_write_half_word
** 函数描述:   往数据流中写入一个半字(16位)数据, 大端模式(高字节先写，低字节后写)
** 参数:       [in] pstrm:     数据流
**             [in] writeword: 写入的数据
** 返回:       无
********************************************************************/
void stream_write_half_word(Stream_t *pstrm, INT16U writeword)
{
    HWORD_UNION temp;
    
    temp.hword = writeword;
    stream_write_byte(pstrm, temp.bytes.high);
    stream_write_byte(pstrm, temp.bytes.low);
}

/*******************************************************************
** 函数名:     stream_write_little_half_word
** 函数描述:   往数据流中写入一个半字(16位)数据, 小端模式(低字节先写，高字节后写)
** 参数:       [in] pstrm:     数据流
**             [in] writeword: 写入的数据
** 返回:       无
********************************************************************/
void stream_write_little_half_word(Stream_t *pstrm, INT16U writeword)
{
    HWORD_UNION temp;
    
    temp.hword = writeword;
    stream_write_byte(pstrm, temp.bytes.low);     
    stream_write_byte(pstrm, temp.bytes.high);
}

/*******************************************************************
** 函数名:     stream_write_long
** 函数描述:   往数据流中写入一个半字(32位)数据, 大端模式(高字节先写，低字节后写)
** 参数:       [in] pstrm:     数据流
**             [in] writeword: 写入的数据
** 返回:       无
********************************************************************/
void stream_write_long(Stream_t *pstrm, INT32U writelong)
{
    stream_write_half_word(pstrm, writelong >> 16);
    stream_write_half_word(pstrm, writelong);
}

/*******************************************************************
** 函数名:     stream_write_little_long
** 函数描述:   往数据流中写入一个半字(32位)数据, 小端模式(低字节先写，高字节后写)
** 参数:       [in] pstrm:     数据流
**             [in] writeword: 写入的数据
** 返回:       无
********************************************************************/
void stream_write_little_long(Stream_t *pstrm, INT32U writelong)
{
    stream_write_little_half_word(pstrm, writelong);
    stream_write_little_half_word(pstrm, writelong >> 16);                     /* 高16位 */
}

/*******************************************************************
** 函数名:     stream_write_linefeed
** 函数描述:   往数据流中写入换行符, 即写入'\r'和'\n'
** 参数:       [in] pstrm: 数据流
** 返回:       无
********************************************************************/
void stream_write_linefeed(Stream_t *pstrm)
{
    stream_write_byte(pstrm, CR);
    stream_write_byte(pstrm, LF);
}

/*******************************************************************
** 函数名:     stream_write_enter
** 函数描述:   往数据流中写入回车符, 即写入'\r''
** 参数:       [in] pstrm: 数据流
** 返回:       无
********************************************************************/
void stream_write_enter(Stream_t *pstrm)
{
    stream_write_byte(pstrm, CR);
}

/*******************************************************************
** 函数名:     stream_write_string
** 函数描述:   往数据流中写入字符串
** 参数:       [in] pstrm: 数据流
**             [in] ptr:   写入的字符串指针
** 返回:       无
********************************************************************/
void stream_write_string(Stream_t *pstrm, char *ptr)
{
    while(*ptr)
    {
        stream_write_byte(pstrm, *ptr++);
    }
}

/*******************************************************************
** 函数名:      stream_write_sprintf
** 函数描述:    数据流实现sprintf功能
** 参数:        [in] pstrm  : 数据流
**              [in] pFormat: 格式化字符串
** 返回:        字符串长度（strlen）
********************************************************************/
INT32S stream_write_sprintf(Stream_t *pstrm, const char* pFormat,...)
{
	va_list arg; 
	INT32S done = 0; 

    if (pstrm == NULL) {
    	return done;
    }

	va_start(arg, pFormat); 
	done += vsprintf((char*)pstrm->current, pFormat, arg); 
	va_end(arg);

    stream_move_pointer(pstrm, done);
	return done;
}

/*******************************************************************
** 函数名:     stream_write_data
** 函数描述:   往数据流中写入一块内存数据
** 参数:       [in] pstrm: 数据流
**             [in] ptr:   写入的数据块地址
**             [in] len:   写入的数据块字节数
** 返回:       无
********************************************************************/
void stream_write_data(Stream_t *pstrm, INT8U *ptr, INT16U len)
{
    while(len--)
    {
        stream_write_byte(pstrm, *ptr++);
    }
}

/*******************************************************************
** 函数名:     stream_write_data_back
** 函数描述:   往数据流中写入一块内存数据,从高地址~低地址，依次存放
** 参数:       [in] pstrm: 数据流
**             [in] ptr:   写入的数据块地址
**             [in] len:   写入的数据块字节数
** 返回:       无
********************************************************************/
void stream_write_data_back(Stream_t *pstrm, INT8U *ptr, INT16U len)
{
    ptr += len - 1;
    while(len--)
    {
        stream_write_byte(pstrm, *ptr--);
    }
}

/*******************************************************************
** 函数名:     stream_read_byte
** 函数描述:   从数据流中读取一个字节
** 参数:       [in] pstrm: 数据流
** 返回:       读取到的字节
********************************************************************/
INT8U stream_read_byte(Stream_t *pstrm)
{
    pstrm->len++;
    return (*pstrm->current++);
}

/*******************************************************************
** 函数名:     stream_read_half_word
** 函数描述:   从数据流中读取一个半字(16位)数据,大端模式(先读为高字节，后读为低字节)
** 参数:       [in] pstrm: 数据流
** 返回:       读取到的字
********************************************************************/
INT16U stream_read_half_word(Stream_t *pstrm)
{
    HWORD_UNION temp = {0};
	
    temp.bytes.high = stream_read_byte(pstrm);
    temp.bytes.low  = stream_read_byte(pstrm);
    return temp.hword;
}

/*******************************************************************
** 函数名:     stream_read_little_half_word
** 函数描述:   从数据流中读取一个半字(16位)数据,小端模式(先读为低字节，后读为高字节)
** 参数:       [in] pstrm: 数据流
** 返回:       读取到的字
********************************************************************/
INT16U stream_read_little_half_word(Stream_t *pstrm)
{
    HWORD_UNION temp = {0};
	
    temp.bytes.low   = stream_read_byte(pstrm);
    temp.bytes.high  = stream_read_byte(pstrm);
    return temp.hword;
}

/*******************************************************************
** 函数名:     stream_read_long
** 函数描述:   从数据流中读取一个半字(32位)数据,大端模式(先读为高字节，后读为低字节)
** 参数:       [in] pstrm: 数据流
** 返回:       读取到的字
********************************************************************/
INT32U stream_read_long(Stream_t *pstrm)
{
    INT32U temp;
	
	temp = (stream_read_half_word(pstrm) << 16);
	temp += stream_read_half_word(pstrm);
    
    return temp;
}

/*******************************************************************
** 函数名:     stream_read_little_long
** 函数描述:   从数据流中读取一个半字(32位)数据,小端模式(先读为低字节，后读为高字节)
** 参数:       [in] pstrm: 数据流
** 返回:       读取到的字
********************************************************************/
INT32U stream_read_little_long(Stream_t *pstrm)
{
    INT32U temp;
	
	temp = stream_read_little_half_word(pstrm);
	temp += (stream_read_little_half_word(pstrm) << 16);
    
    return temp;
}

/*******************************************************************
** 函数名:     stream_read_data
** 函数描述:   从数据流中读取指定长度的数据内容
** 参数:       [in]  pstrm: 数据流
**             [out] ptr:   读取到的数据存放的内存地址
**             [in]  len:   读取的数据长度
** 返回:       无
********************************************************************/
void stream_read_data(Stream_t *pstrm, INT8U *ptr, INT16U len)
{
    while(len--)
    {
        *ptr++ = stream_read_byte(pstrm);
    }
}

/*******************************************************************
** 函数名:     stream_read_data_back
** 函数描述:   从数据流中读取指定长度的数据内容,从高地址~低地址，依次存放
** 参数:       [in]  pstrm: 数据流
**             [out] ptr:   读取到的数据存放的内存地址
**             [in]  len:   读取的数据长度
** 返回:       无
********************************************************************/
void stream_read_data_back(Stream_t *pstrm, INT8U *ptr, INT16U len)
{
    ptr += len - 1;
    while(len--)
    {
        *ptr-- = stream_read_byte(pstrm);
    }
}

