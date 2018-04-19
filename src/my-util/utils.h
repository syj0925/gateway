/********************************************************************************
**
** 文件名:     utils.h
** 版权所有:   
** 文件描述:   工具函数
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2012/08/30 | 苏友江 |  创建该文件
*********************************************************************************/
#ifndef _UTILS_H_
#define _UTILS_H_

typedef struct {
    INT8U id;
    INT8U namelen;
    char *name;
} String_tab_t;

typedef struct {
    const String_tab_t *tab;
    INT8U tabid;
    INT8U tabnum;
    INT8U index[10];                                                           /* 必须大于tab数组个数，根据实际需求调整 */
} String_ident_t;


/*******************************************************************
** 函数名:      string_ident
** 函数描述:    根据字符串列表，查询匹配的字符串
** 参数:        [in] pident: 识别对象
**              [in] ppstr : 待识别数据，识别成功，返回剩余数据地址
**              [in] plen  : 待识别数据长度，识别成功，返回剩余数据长度
** 返回:        -1指未匹配，>=0 指匹配到指定列表索引
********************************************************************/
INT32S string_ident(String_ident_t *pident, INT8U **ppstr, INT32U *plen);

/*******************************************************************
** 函数名:     u_chksum_1
** 函数描述:   单字节校验码算法
** 参数:       [in] ptr:      目标数据地址
**             [in] len:      目标数据长度
** 返回:       单字节校验码
********************************************************************/
INT8U u_chksum_1(INT8U *ptr, INT32U len);

/*******************************************************************
** 函数名:     u_chksum_1b
** 函数描述:   单字节校验码算法,数据取反码后再计算校验和
** 参数:       [in] ptr:      目标数据地址
**             [in] len:      目标数据长度
** 返回:       单字节校验码
********************************************************************/
INT8U u_chksum_1b(INT8U *ptr, INT32U len);

/*******************************************************************
** 函数名:     u_chksum_2
** 函数描述:   双字节校验码算法
** 参数:       [in] ptr:      目标数据地址
**             [in] len:      目标数据长度
** 返回:       双字节校验码
********************************************************************/
INT16U u_chksum_2(INT8U *ptr, INT32U len);

/*******************************************************************
** 函数名:     chksum_xor
** 函数描述:   获取数据区异或校验码
** 参数:       [in] ptr: 数据区地址
**             [in] len: 数据区长度
** 返回:       单字节带异或校验码
********************************************************************/
INT8U chksum_xor(INT8U *ptr, INT32U len);

INT8U bcd_to_hex_byte(INT8U bcd);

INT8U hex_to_bcd_byte(INT8U hex);

void bcd_to_hex(INT8U *dptr, INT8U *sptr, INT16U len);

void *mem_find_char(void *addr, INT32S size, INT8U c);

INT8U *mem_find(INT8U *addr, INT32S size, INT8U *fpart, INT32S flen);

INT8U* mem_find_str(INT8U *addr, INT32S size, char *fpart);

/*******************************************************************
** 函数名:      deassemble_by_rules
** 函数描述:    按照指定规则解析帧数据
** 参数:        [in] dptr : 目标指针
**              [in] sptr : 源指针
**              [in] len  : 源数据长度
**              [in] rule : 规则
** 返回:        解析后实际长度
********************************************************************/
INT16U deassemble_by_rules(INT8U *dptr, INT8U *sptr, INT16U len, Asmrule_t *rule);

/*******************************************************************
** 函数名:      deassemble_by_rules
** 函数描述:    按照指定规则封装帧数据
** 参数:        [in] dptr : 目标指针
**              [in] sptr : 源指针
**              [in] len  : 源数据长度
**              [in] rule : 规则
** 返回:        封装实际长度
********************************************************************/
INT16U assemble_by_rules(INT8U *dptr, INT8U *sptr, INT16U len, Asmrule_t *rule);


INT32S ut_crc16(INT8U *crc,INT8U *puchMsg, INT32S usDataLen);

INT32S ut_crc16_separate(INT8U *crc,INT8U *puchMsg, INT32S usDataLen);

INT16U htons(INT16U n);

INT16U ntohs(INT16U n);

INT32U htonl(INT32U n);

INT32U ntohl(INT32U n);

INT16U inet_chksum(void *dataptr, INT16U len);

/*******************************************************************
** 函数名:     ParseIpAddr
** 函数描述:   解析IP地址,如ip_string为：11.12.13.14，则转换后的ip_long为0x0B0C0D0E
** 参数:       [out] ip_long:  ip地址，依次从从高位到低位
**             [out] sbits:    子网位数
**             [in]  ip_string:ip字符串，以'\0'为结束符，如“11.12.13.14”
** 返回:       转换成功返回0，失败返回失败描述符
********************************************************************/
char *ParseIpAddr(INT32U *ip_long, INT8U *sbits, char *ip_string);


#endif /* _UTILS_H_ */
