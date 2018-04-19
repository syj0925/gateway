/********************************************************************************
**
** 文件名:     mem.h
** 版权所有:   
** 文件描述:   实现定时器驱动
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2012/08/14 | 苏友江 |  创建该文件
*********************************************************************************/
#ifndef __MEM_H__
#define __MEM_H__


#ifdef __cplusplus
extern "C" {
#endif

#define OS_NOASSERT 0
#if     OS_NOASSERT
#define OS_ASSERT(x,y) do { if(!(y)) OS_PLATFORM_ASSERT(x); } while(0)
#else  /* OS_NOASSERT */
#define OS_ASSERT(x,y) 
#endif /* OS_NOASSERT */



#ifndef MEM_ALIGNMENT                   /* ------------------ 字对齐配置跟处理器相关 ------------------ */
#define MEM_ALIGNMENT             4     /* 本次采用的是ARM7TDMI处理器，因此是4字节对齐的 -------------- */
#endif

#ifndef MEM_SIZE
#define MEM_SIZE                  2*1024/* 动态内存堆(HEAP)分配策略,为动态内存分配大小 ---------------- */
#endif

#ifndef MEM_MOVE_UP
#define MEM_MOVE_UP               1
#endif

#ifndef MEM_MOVE_DOWN
#define MEM_MOVE_DOWN             0
#endif

typedef INT32U               mem_ptr_t;
/* MEM_SIZE would have to be aligned, but using 64000 here instead of
 * 65535 leaves some room for alignment...
 */
#if MEM_SIZE > 64000l
typedef INT32U mem_size_t;
#else
typedef INT16U mem_size_t;
#endif /* MEM_SIZE > 64000 */


#if 0
typedef struct packet_buf_t {
    /** next pbuf in singly linked pbuf chain */
    struct packet_buf_t *next;
    void  *curptr;
    INT16U free_len;
    INT16U used_len;  
    INT16U tot_len;
    INT8U  state;
}PACKET_BUF_T;
#endif /* if 0. 2014-10-18 10:55:36 suyoujiang */

/* lwIP alternative malloc */
void  mem_init(void);
void *mem_malloc(mem_size_t size);
void *mem_realloc(void *rmem, mem_size_t newsize);
void *mem_calloc(mem_size_t count, mem_size_t size);
void  mem_free(void *mem);
char *mem_strdup(const char *s);

#if 0
/*******************************************************************
** 函数名:     pbuf_alloc
** 函数描述:   分配包缓存动态内存
** 参数:       [in] size:       目前要用的大小 
** 参数:       [in] freesize:   预留部分的大小
** 返回:       动态包内存首地址
** 注意:       
********************************************************************/
PACKET_BUF_T *pbuf_alloc(mem_size_t size, mem_size_t freesize);

/*******************************************************************
** 函数名:     pbuf_move
** 函数描述:   移动动态内存包的数据指针
** 参数:       [in] pbuf:   要操作的动态内存包的指针 
** 参数:       [in] size:   移动的长度
** 参数:       [in] dir:    移动的方向
** 返回:       true or false
** 注意:       
********************************************************************/
BOOLEAN pbuf_move(PACKET_BUF_T *pbuf, mem_size_t size, BOOLEAN dir);
#endif /* if 0. 2014-10-18 10:55:26 suyoujiang */

#ifndef OS_MEM_ALIGN_SIZE
#define OS_MEM_ALIGN_SIZE(size) (((size) + MEM_ALIGNMENT - 1) & ~(MEM_ALIGNMENT-1))
#endif

#ifndef OS_MEM_ALIGN
#define OS_MEM_ALIGN(addr) ((void *)(((mem_ptr_t)(addr) + MEM_ALIGNMENT - 1) & ~(mem_ptr_t)(MEM_ALIGNMENT-1)))
#endif

#ifdef __cplusplus
}
#endif

#endif /* __MEM_H__ */
