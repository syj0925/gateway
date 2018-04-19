/********************************************************************************
**
** 文件名:     roundbuf.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   实现循环缓冲区管理功能
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2007/04/11 | 陈从华 |  创建该文件
**| 2008/10/13 | 任赋   |  使用原车台程序实现和接口,方便车台程序移植
*********************************************************************************/
#include "sys_typedef.h"
#include "roundbuf.h"
/*******************************************************************
** 函数名:     roundbuf_init
** 函数描述:   初始化循环缓冲区
** 参数:       [in]  round:           循环缓冲区
**             [in]  mem:             循环缓冲区所管理的内存地址
**             [in]  memsize:         循环缓冲区所管理的内存字节数
**             [in]  rule:            循环缓冲区数据读取是的解码规则
** 返回:       无
********************************************************************/
void roundbuf_init(RoundBuf_t *round, INT8U *mem, INT32U memsize, AsmRule_t *rule)
{
    round->bufsize = memsize;
    round->used    = 0;
    round->bptr    = mem;
    round->eptr    = mem + memsize;
    round->wptr = round->rptr = round->bptr;
    round->rule = rule;
}

/*******************************************************************
** 函数名:     roundbuf_reset
** 函数描述:   复位循环缓冲区, 即将循环缓冲区的已使用字节数清0
** 参数:       [in]  round:           循环缓冲区
** 返回:       无
********************************************************************/
void roundbuf_reset(RoundBuf_t *round)
{
    round->used = 0;
    round->rptr = round->wptr = round->bptr;
}

/*******************************************************************
** 函数名:     roundbuf_get_start_pos
** 函数描述:   获取循环缓冲区所管理内存的起始指针
** 参数:       [in]  round:           循环缓冲区
** 返回:       所管理内存的起始指针
********************************************************************/
INT8U *roundbuf_get_start_pos(RoundBuf_t *round)
{
    return round->bptr;
}

/*******************************************************************
** 函数名:     roundbuf_write_byte
** 函数描述:   往循环缓冲区中写入一个字节的数据
** 参数:       [in]  round:           循环缓冲区
**             [in]  data:              待写入的数据
** 返回:       如写之前循环缓冲区中的使用字节数达到所管理内存的字节数, 
**             则返回失败; 否则, 返回成功
********************************************************************/
BOOLEAN roundbuf_write_byte(RoundBuf_t *round, INT8U data)
{
    if(NULL == round) return FALSE;
    if (round->used >= round->bufsize) return FALSE;
    *round->wptr++ = data;
    if (round->wptr >= round->eptr) {
        round->wptr = round->bptr;
    }
    round->used++;
    return TRUE;
}

/*******************************************************************
** 函数名:     roundbuf_read_byte
** 函数描述:   从循环缓冲区中读取一个字节的数据
** 参数:       [in]  round:           循环缓冲区
** 返回:       如读之前循环缓冲区中的使用字节数为0, 则返回-1;
**             否则, 返回读取到的字节
********************************************************************/
INT32S roundbuf_read_byte(RoundBuf_t *round)
{
    INT32S ret;
    
    if (round->used == 0) return -1;
    ret = *round->rptr++;
    if (round->rptr >= round->eptr) {
        round->rptr = round->bptr;
    }
    round->used--;
    return ret;
}

/*******************************************************************
** 函数名:     roundbuf_read_data
** 函数描述:   读循环缓冲区指定长度
** 参数:       [in]  round:           循环缓冲区
**             [out] pdata:           要读数据块的指针
**             [in]  datalen:         要读数据块的字节数
** 返回:       实际读出数据长度
********************************************************************/
INT32S roundbuf_read_data(RoundBuf_t *round, INT8U *pdata, INT32U datalen)
{
    INT32U i, readlen;

    if (round->used <= datalen) {
        readlen = round->used;
    } else {
        readlen = datalen;
    }

    for (i = 0; i < readlen; i++) {

        pdata[i] = *round->rptr++;
        if (round->rptr >= round->eptr) {
            round->rptr = round->bptr;
        }
        round->used--;
    }
    
    return readlen;
}

/*******************************************************************
** 函数名:     roundbuf_read_byte_no_move_ptr
** 函数描述:   再不移动缓冲区指针的情况下,从循环缓冲区中读取一个字节的数据
** 参数:       [in]  round:           循环缓冲区
** 返回:       剩余的可用字节数
********************************************************************/
INT32S roundbuf_read_byte_no_move_ptr(RoundBuf_t *round)
{
    if (round->used == 0) return -1;
    else return *round->rptr; 
}

/*******************************************************************
** 函数名:     roundbuf_get_left
** 函数描述:   获取循环缓冲区中剩余的可用字节数
** 参数:       [in]  round:           循环缓冲区
** 返回:       剩余的可用字节数
********************************************************************/
INT32U roundbuf_get_left(RoundBuf_t *round)
{
    return (round->bufsize - round->used);
}

/*******************************************************************
** 函数名:     roundbuf_get_used
** 函数描述:   获取循环缓冲区中已使用字节数
** 参数:       [in]  round:           循环缓冲区
** 返回:       已使用的字节数
********************************************************************/
INT32U roundbuf_get_used(RoundBuf_t *round)
{
       return round->used;
}

/*******************************************************************
** 函数名:     roundbuf_write_block
** 函数描述:   往循环缓冲区中写入一块数据单元
** 参数:       [in]  round:           循环缓冲区
**             [in]  bptr:            待写入数据块的指针
**             [in]  blksize:         待写入数据块的字节数
** 返回:       成功写入循环缓冲区的字节数
********************************************************************/
BOOLEAN roundbuf_write_block(RoundBuf_t *round, INT8U *bptr, INT32U blksize)
{
    if (blksize > roundbuf_get_left(round)) return FALSE;
    for (; blksize > 0; blksize--) { 
       *round->wptr++ = *bptr++;
       if (round->wptr >= round->eptr) round->wptr = round->bptr;
       round->used++;
    }    
    return TRUE;
}

/*******************************************************************
** 函数名:     roundbuf_deassemble
** 函数描述:   读取缓冲区中的数据并按照规则进行解码
** 参数:       [in]  round:           循环缓冲区
**             [in]  bptr:            待读入数据块的指针
**             [in]  blksize:         待读取数据块的最大字节数
** 返回:       读取的解码后的字节数
********************************************************************/
INT32U roundbuf_deassemble(RoundBuf_t *round, INT8U *dptr, INT32U maxlen)
{
    INT8U  *rptr, *tptr;
    INT8U  cur, pre = 0;
    INT32U used, ct;
    INT32S rlen;
    
    if (round->rule == NULL) return 0;
    
    rlen = -1;
    ct   = 0;
    tptr = dptr;
    rptr = round->rptr;
    used = round->used;
    while (used > 0) {
        cur = *rptr++;
        used--;
        ct++;
        if (rptr >= round->eptr) rptr = round->bptr;
        if (rlen == -1) {                                   /* search flags */
            if (cur == round->rule->c_flags) {
                rlen = 0;
                pre  = 0;
            }
            continue;
        } else if (rlen == 0) {
            if (cur == round->rule->c_flags) continue;
        } else {
            if (cur == round->rule->c_flags) {
                round->rptr  = rptr;
                round->used -= ct;
                return rlen;
            }
        }
        if (pre == round->rule->c_convert0) {               /* search convert character */
            if (cur == round->rule->c_convert1)             /* c_flags    = c_convert0 + c_convert1 */
                *tptr++ = round->rule->c_flags;
            else if (cur == round->rule->c_convert2)        /* c_convert0 = c_convert0 + c_convert2 */
                *tptr++ = round->rule->c_convert0;
            else {
                round->rptr  = rptr;                        /* detect error frame! */
                round->used -= ct;
                ct = 0;
                tptr = dptr;
                rlen = -1;
                continue;
            }
        } else {
            if (cur != round->rule->c_convert0)             /* search convert character */
                *tptr++ = cur;
            rlen++;
        }
        pre = cur;
        if (rlen >= maxlen) {
            round->rptr  = rptr;
            round->used -= ct;
            ct = 0;
            tptr = dptr;
            rlen = -1;
            continue;
        }
    }
    return 0;
}

