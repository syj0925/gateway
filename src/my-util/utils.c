/********************************************************************************
**
** 文件名:     utils.c
** 版权所有:   
** 文件描述:   工具函数
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2012/08/14 | 苏友江 |  创建该文件
*********************************************************************************/
#include "sys_typedef.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"
/*******************************************************************
** 函数名:      string_ident
** 函数描述:    根据字符串列表，查询匹配的字符串
** 参数:        [in] pident: 识别对象
**              [in] ppstr : 待识别数据，识别成功，返回剩余数据地址
**              [in] plen  : 待识别数据长度，识别成功，返回剩余数据长度
** 返回:        -1指未匹配，>=0 指匹配到指定列表索引
********************************************************************/
INT32S string_ident(String_ident_t *pident, INT8U **ppstr, INT32U *plen)
{
    INT8U *pstr;
    INT32U i, j, len;
    const String_tab_t *ptab;
    
    len  = *plen;
    pstr = *ppstr;
    ptab = pident->tab;
    
    for (i = 0; i < len; i++) {

        for (j = 0; j < pident->tabnum; j++) {
            if (ptab[j].name[pident->index[j]] == pstr[i]) {
                pident->index[j]++;
            } else {
                pident->index[j] = 0;
                continue;
            }

            if (pident->index[j] >= ptab[j].namelen) {
                pident->tabid = j;
                memset(pident->index, 0, pident->tabnum);
                *ppstr += (i + 1);
                *plen  -= (i + 1);
                return pident->tabid;
            }
        }
    }

    *plen = 0;
    return -1;
}

/*******************************************************************
** 函数名:     u_chksum_1
** 函数描述:   单字节校验码算法
** 参数:       [in] ptr:      目标数据地址
**             [in] len:      目标数据长度
** 返回:       单字节校验码
********************************************************************/
INT8U u_chksum_1(INT8U *ptr, INT32U len)
{
    INT32U i;
    HWORD_UNION chksum;
    
    chksum.hword = 0;
    for (i = 1; i <= len; i++) {
        chksum.hword += *ptr++;
        if (chksum.bytes.high > 0) {
            chksum.bytes.low += chksum.bytes.high;
            chksum.bytes.high = 0;
        }
    }
    chksum.bytes.low += 1;
    return chksum.bytes.low;
}
/*******************************************************************
** 函数名:     u_chksum_1b
** 函数描述:   单字节校验码算法,数据取反码后再计算校验和
** 参数:       [in] ptr:      目标数据地址
**             [in] len:      目标数据长度
** 返回:       单字节校验码
********************************************************************/
INT8U u_chksum_1b(INT8U *ptr, INT32U len)
{
    INT32U i;
    HWORD_UNION chksum;
    
    chksum.hword = 0;
    for (i = 1; i <= len; i++) {
        chksum.hword += (INT8U)(~(*ptr++));
        if (chksum.bytes.high > 0) {
            chksum.bytes.low += chksum.bytes.high;
            chksum.bytes.high = 0;
        }
    }
    chksum.bytes.low += 1;
    return chksum.bytes.low;
}

/*******************************************************************
** 函数名:     u_chksum_2
** 函数描述:   双字节校验码算法
** 参数:       [in] ptr:      目标数据地址
**             [in] len:      目标数据长度
** 返回:       双字节校验码
********************************************************************/
INT16U u_chksum_2(INT8U *ptr, INT32U len)
{
    INT8U  temp;
    INT16U chksum;
    
    temp    = u_chksum_1(ptr, len);
    chksum  = (INT16U)temp << 8;
    temp    = u_chksum_1b(ptr, len);
    chksum |= temp;
    return chksum;
}

/*******************************************************************
** 函数名:     chksum_xor
** 函数描述:   获取数据区异或校验码
** 参数:       [in] ptr: 数据区地址
**             [in] len: 数据区长度
** 返回:       单字节带异或校验码
********************************************************************/
INT8U chksum_xor(INT8U *ptr, INT32U len)
{
    INT8U sum;
   
    sum = 0;
    while (len--) {
        sum ^= *ptr++;
    }
   
    return sum;
}

INT8U bcd_to_hex_byte(INT8U bcd)
{
    INT8U temp;
    
    temp = (bcd >> 4) * 10 + (bcd & 0x0f);
    
    return temp;
}

INT8U hex_to_bcd_byte(INT8U hex)
{
	INT8U stemp, dtemp, bcddata;

	stemp   = hex%100;
	dtemp   = stemp/10;
	bcddata = dtemp<<4;
	dtemp   = stemp%10;
	bcddata += dtemp;
	return bcddata;
}

void bcd_to_hex(INT8U *dptr, INT8U *sptr, INT16U len)
{
    INT16U i;
    
    for (i = 0; i < len; i++) {
        *dptr++ = bcd_to_hex_byte(*sptr++);
    }
}

void *mem_find_char(void *addr, INT32S size, INT8U c)
{
	INT8U *p = addr;

	while (size > 0) {
		if (*p == c)
			return (void *)p;
		p++;
		size--;
	}
  	return NULL;
}

INT8U *mem_find(INT8U *addr, INT32S size, INT8U *fpart, INT32S flen)
{
	INT8U* rlt = NULL;
	INT8U *p = addr;

	if (size > 0 && flen > 0) {
		while ((rlt = mem_find_char(p, addr + size - p, fpart[0])) != NULL) {
			INT32U i;
			INT8U nmatch = 0;
            
			p = rlt;
			for (i = 0; i < flen; i++) {
				if (*p++ != fpart[i]) {
					nmatch = 1;
					break;
				}
			}

			if (nmatch == 0)
				break;
			p--;
		}
	}
  	return rlt;
}

INT8U* mem_find_str(INT8U *addr, INT32S size, char *fpart)
{
	return mem_find(addr, size, (INT8U *)fpart, strlen(fpart));
}

/*******************************************************************
** 函数名:      deassemble_by_rules
** 函数描述:    按照指定规则解析帧数据
** 参数:        [in] dptr : 目标指针
**              [in] sptr : 源指针
**              [in] len  : 源数据长度
**              [in] rule : 规则
** 返回:        解析后实际长度
********************************************************************/
INT16U deassemble_by_rules(INT8U *dptr, INT8U *sptr, INT16U len, Asmrule_t *rule)
{
    INT16U rlen;
    INT8U  prechar, curchar, c_convert0;
    
    if (rule == NULL) return 0;
    c_convert0 = rule->c_convert0;
    rlen = 0;
    prechar = (~c_convert0);
    for (; len > 0; len--) {
        curchar = *sptr++;
        if (prechar == c_convert0) {
            if (curchar == rule->c_convert1) {            /* c_convert0 + c_convert1 = c_flags */
                prechar = (~c_convert0);
                *dptr++ = rule->c_flags;
                rlen++;
            } else if (curchar == rule->c_convert2) {     /* c_convert0 + c_convert2 = c_convert0 */
                prechar = (~c_convert0);
                *dptr++ = c_convert0;
                rlen++;
            } else {
                return 0;
            }
        } else {
            if ((prechar = curchar) != c_convert0) {
                *dptr++ = curchar;
                rlen++;
            }
        }
    }
    return rlen;
}

/*******************************************************************
** 函数名:      deassemble_by_rules
** 函数描述:    按照指定规则封装帧数据
** 参数:        [in] dptr : 目标指针
**              [in] sptr : 源指针
**              [in] len  : 源数据长度
**              [in] rule : 规则
** 返回:        封装实际长度
********************************************************************/
INT16U assemble_by_rules(INT8U *dptr, INT8U *sptr, INT16U len, Asmrule_t *rule)
{
    INT8U  temp;
    INT16U rlen;
    
    if (rule == NULL) return 0;
    
    *dptr++ = rule->c_flags;
    rlen    = 1;
    for (; len > 0; len--) {
        temp = *sptr++;
        if (temp == rule->c_flags) {            /* c_flags    = c_convert0 + c_convert1 */
            *dptr++ = rule->c_convert0;
            *dptr++ = rule->c_convert1;
            rlen += 2;
        } else if (temp == rule->c_convert0) {  /* c_convert0 = c_convert0 + c_convert2 */
            *dptr++ = rule->c_convert0;
            *dptr++ = rule->c_convert2;
            rlen += 2;
        } else {
            *dptr++ = temp;
            rlen++;
        }
    }
    *dptr = rule->c_flags;
    rlen++;
    return rlen;
}

static const INT8U auchCRCHi[]={
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 
	0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 
	0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 
	0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 
	0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 
	0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 
	0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 
	0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 
	0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
	0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 
	0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 
	0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 
	0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 
	0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 
	0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 
	0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 
	0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 
	0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 
	0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 
	0x80, 0x41, 0x00, 0xC1, 0x81, 0x40 
};

static const INT8U auchCRCLo[]={
	0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 
	0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD, 
	0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09, 
	0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A, 
	0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC, 0x14, 0xD4, 
	0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3, 
	0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 
	0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4, 
	0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A, 
	0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29, 
	0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED, 
	0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26, 
	0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 
	0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67, 
	0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F, 
	0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68, 
	0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E, 
	0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5, 
	0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 
	0x70, 0xB0, 0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92, 
	0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C, 
	0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B, 
	0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B, 
	0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C, 
	0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 
	0x43, 0x83, 0x41, 0x81, 0x80, 0x40 
};

INT32S ut_crc16(INT8U *crc,INT8U *puchMsg, INT32S usDataLen)
{ 
	INT8U uchCRCHi = 0xFF ; /* 低CRC 字节初始化 */ 
	INT8U uchCRCLo = 0xFF ; 
	INT32U uIndex ;			/* CRC循环中的索引 */ 
	if(usDataLen<=0) return -1;
	while (usDataLen--)		/* 传输消息缓冲区 */
	{ 
		uIndex = uchCRCHi ^ *puchMsg++ ; /* 计算CRC */ 
		uchCRCHi = uchCRCLo ^ auchCRCHi[uIndex] ; /* 计算CRC 高低字节对调*/
		uchCRCLo = auchCRCLo[uIndex] ; 
	}  
	
	crc[1] = uchCRCHi;
	crc[0] = uchCRCLo;
	return 0;
} 

INT32S ut_crc16_separate(INT8U *crc,INT8U *puchMsg, INT32S usDataLen)
{ 
	INT8U uchCRCHi = 0xFF ; /* 低CRC 字节初始化 */ 
	INT8U uchCRCLo = 0xFF ; 
	INT32U uIndex ;			/* CRC循环中的索引 */ 

	uchCRCHi = crc[1];
	uchCRCLo = crc[0];
	if(usDataLen<=0) return -1;
	while (usDataLen--)		/* 传输消息缓冲区 */
	{ 
		uIndex = uchCRCHi ^ *puchMsg++ ; /* 计算CRC */ 
		uchCRCHi = uchCRCLo ^ auchCRCHi[uIndex] ; /* 计算CRC 高低字节对调*/
		uchCRCLo = auchCRCLo[uIndex] ; 
	}  
	
	crc[1] = uchCRCHi;
	crc[0] = uchCRCLo;
	return 0;
} 


/**
 * Convert an u16_t from host- to network byte order.
 *
 * @param n u16_t in host byte order
 * @return n in network byte order
 */
INT16U htons(INT16U n)
{
    return ((n & 0xff) << 8) | ((n & 0xff00) >> 8);
}

/**
 * Convert an u16_t from network- to host byte order.
 *
 * @param n u16_t in network byte order
 * @return n in host byte order
 */
INT16U ntohs(INT16U n)
{
    return ((n & 0xff) << 8) | ((n & 0xff00) >> 8);
}

/**
 * Convert an u32_t from host- to network byte order.
 *
 * @param n u32_t in host byte order
 * @return n in network byte order
 */
INT32U htonl(INT32U n)
{
	return ((n & 0xff) << 24) |
		((n & 0xff00) << 8) |
		((n & 0xff0000UL) >> 8) |
		((n & 0xff000000UL) >> 24);
}

/**
 * Convert an u32_t from network- to host byte order.
 *
 * @param n u32_t in network byte order
 * @return n in host byte order
 */
INT32U ntohl(INT32U n)
{
    return htonl(n);
}


/**
 * lwip checksum
 *
 * @param dataptr points to start of data to be summed at any boundary
 * @param len length of data to be summed
 * @return host order (!) lwip checksum (non-inverted Internet sum) 
 *
 * @note accumulator size limits summable length to 64k
 * @note host endianess is irrelevant (p3 RFC1071)
 */
static INT16U lwip_standard_chksum(void *dataptr, INT16U len)
{
    INT32U acc;
    INT16U src;
    INT8U *octetptr;

    acc = 0;
    /* dataptr may be at odd or even addresses */
    octetptr = (INT8U*)dataptr;
    while (len > 1)
    {
        /* declare first octet as most significant
        thus assume network order, ignoring host order */
        src = (*octetptr) << 8;
        octetptr++;
        /* declare second octet as least significant */
        src |= (*octetptr);
        octetptr++;
        acc += src;
        len -= 2;
    }
    if (len > 0)
    {
        /* accumulate remaining octet */
        src = (*octetptr) << 8;
        acc += src;
    }
    /* add deferred carry bits */
    acc = (acc >> 16) + (acc & 0x0000ffffUL);
    if ((acc & 0xffff0000) != 0) {
        acc = (acc >> 16) + (acc & 0x0000ffffUL);
    }
    /* This maybe a little confusing: reorder sum using htons()
    instead of ntohs() since it has a little less call overhead.
    The caller must invert bits for Internet sum ! */
    return htons((INT16U)acc);
}

/* inet_chksum:
 *
 * Calculates the Internet checksum over a portion of memory. Used primarily for IP
 * and ICMP.
 *
 * @param dataptr start of the buffer to calculate the checksum (no alignment needed)
 * @param len length of the buffer to calculate the checksum
 * @return checksum (as u16_t) to be saved directly in the protocol header
 */
INT16U inet_chksum(void *dataptr, INT16U len)
{
    INT32U acc;

    acc = lwip_standard_chksum(dataptr, len);
    while ((acc >> 16) != 0) {
        acc = (acc & 0xffff) + (acc >> 16);
    }
    return (INT16U)~(acc & 0xffff);
}

/*******************************************************************
** 函数名:     ParseIpAddr
** 函数描述:   解析IP地址,如ip_string为：11.12.13.14，则转换后的ip_long为0x0B0C0D0E
** 参数:       [out] ip_long:  ip地址，依次从从高位到低位
**             [out] sbits:    子网位数
**             [in]  ip_string:ip字符串，以'\0'为结束符，如“11.12.13.14”
** 返回:       转换成功返回0，失败返回失败描述符
********************************************************************/
char *ParseIpAddr(INT32U *ip_long, INT8U *sbits, char *ip_string)
{
    char *cp;
    INT8U dots = 0;
    INT32U number;
    LONG_UNION retval = {0};
    char *toobig = "each number must be less than 255";

    cp = ip_string;
    while (*cp) {
        if (*cp > '9' || *cp < '.' || *cp == '/') {
            return("all chars must be digits (0-9) or dots (.)");
        }
        if (*cp == '.') {
            dots++;
        }
        cp++;
    }

    if (dots != 3 ) {
        return("string must contain 3 dots (.)");
    }

    cp = ip_string;                                                             /* 第一字节 */
    number = atoi(cp);
    if (number > 255) {
        return(toobig);
    }
    retval.bytes.byte1 = (INT8U)number;

    while (*cp != '.') {                                                       /* 第二字节 */
        cp++;
    }
    cp++;
    number = atoi(cp);
    if (number > 255) {
        return(toobig);
    }
    retval.bytes.byte2 = (INT8U)number;
    
    while (*cp != '.') {                                                       /* 第三字节 */
        cp++;
    }
    cp++;
    number = atoi(cp);
    if (number > 255) {
        return(toobig);
    }
    retval.bytes.byte3 = (INT8U)number;
    
    while (*cp != '.') {                                                       /* 第四字节 */
        cp++;
    }
    cp++;
    number = atoi(cp);
    if (number > 255) {
        return(toobig);
    }
    retval.bytes.byte4 = (INT8U)number;

    if (retval.bytes.byte1 < 128) *sbits = 8;
    else if(retval.bytes.byte1 < 192) *sbits = 16;
    else *sbits = 24;

    *ip_long = retval.ulong;
    return (NULL);
}

