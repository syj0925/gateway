/********************************************************************************
**
** 文件名:     sys_typedef.h
** 版权所有:   (c) 2013-2014 
** 文件描述:   定义宏和数据类型
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**|    日期    |  作者  |  修改记录
**===============================================================================
**| 2014/10/13 | 苏友江 |  创建该文件
********************************************************************************/
#ifndef SYS_TYPEDEF_H
#define SYS_TYPEDEF_H

/*
********************************************************************************
* 定义宏
********************************************************************************
*/
#ifndef TRUE
#define TRUE  1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef true
#define true  1
#endif

#ifndef false
#define false 0
#endif

#ifndef NULL
#define NULL  ((void *)0)
#endif

typedef enum {
	NO = 0,
    YES,
    ERR
} comfirm_enum_t;

#define PACKED __attribute__ ((packed))          /* __packed */


/*
********************************************************************************
* 重定义一些基本数据类型
********************************************************************************
*/
typedef unsigned char  BOOLEAN;
typedef unsigned char  INT8U;                    /* Unsigned  8 bit quantity                           */
typedef signed   char  INT8S;                    /* Signed    8 bit quantity                           */
typedef unsigned short INT16U;                   /* Unsigned 16 bit quantity                           */
typedef signed   short INT16S;                   /* Signed   16 bit quantity                           */
typedef unsigned int   INT32U;                   /* Unsigned 32 bit quantity                           */
typedef signed   int   INT32S;                   /* Signed   32 bit quantity                           */
typedef float          FP32;                     /* Single precision floating point                    */
typedef double         FP64;                     /* Double precision floating point                    */

typedef unsigned char  uint8;                   /* defined for unsigned 8-bits integer variable 	无符号8位整型变量  */
typedef signed   char  int8;                    /* defined for signed 8-bits integer variable		有符号8位整型变量  */
typedef unsigned short uint16;                  /* defined for unsigned 16-bits integer variable 	无符号16位整型变量 */
typedef signed   short int16;                   /* defined for signed 16-bits integer variable 		有符号16位整型变量 */
typedef unsigned int   uint32;                  /* defined for unsigned 32-bits integer variable 	无符号32位整型变量 */
typedef signed   int   int32;                   /* defined for signed 32-bits integer variable 		有符号32位整型变量 */

typedef unsigned char	  BYTE;
typedef unsigned short    WORD;
typedef unsigned long     DWORD;
typedef unsigned int      BOOL;

typedef unsigned int      size_t;

/*
********************************************************************************
* DEFINE WORD UNION
********************************************************************************
*/
typedef union {
	INT16U     hword;
#ifdef __BIG_ENDIAN
	struct {
		INT8U  high;
		INT8U  low;
	} bytes;
#else
	struct {
		INT8U  low;
		INT8U  high;
	} bytes;
#endif
} HWORD_UNION;

/*
********************************************************************************
* DEFINE LONG UNION
********************************************************************************
*/
typedef union {
    INT32U ulong;
#ifdef  __BIG_ENDIAN   
    struct {
		INT8U  byte1;
		INT8U  byte2;
        INT8U  byte3;
		INT8U  byte4;
	} bytes;
#else
    struct {
		INT8U  byte4;
		INT8U  byte3;
        INT8U  byte2;
		INT8U  byte1;
	} bytes;
#endif
} LONG_UNION;


typedef struct {
    INT8U       c_flags;                /* c_convert0 + c_convert1 = c_flags */
    INT8U       c_convert0;             /* c_convert0 + c_convert2 = c_convert0 */
    INT8U       c_convert1;
    INT8U       c_convert2;
} Asmrule_t;

/*
********************************************************************************
*                  DEFINE DATE STRUCT
********************************************************************************
*/
typedef struct {
    INT8U           year;
    INT8U           month;
    INT8U           day;
} Date_t;

/*
********************************************************************************
*                  DEFINE TIME STRUCT
********************************************************************************
*/
typedef struct {
    INT8U           hour;
    INT8U           minute;
    INT8U           second;
} Time_t;

/*
********************************************************************************
*                  DEFINE SYSTIME STRUCT
********************************************************************************
*/
#if 0
typedef struct {
    Date_t     date;
    Time_t     time;
} Systime_t;
#endif /* if 0. 2015-9-15 23:02:15 suyoujiang */

typedef enum _Ret
{
	RET_OK,
	RET_FAIL,
    RET_INVALID_PARAMS,
	RET_OUT_OF_SPACE
} Ret;

#define FTK_CALL_LISTENER(listener, u, o) listener != NULL ? listener(u, o) : RET_OK
#define return_if_fail(p) if(!(p)) { printf("%s:%d "#p" failed.\n", __func__, __LINE__); return;}
#define return_val_if_fail(p, val) if(!(p)) {printf("%s:%d "#p" failed.\n", __func__, __LINE__); return (val);}
#define DECL_PRIV(thiz, priv)  Priv_t* priv = thiz != NULL ? (Priv_t*)thiz->priv : NULL
#define DECL_PRIV0(thiz, priv) Priv_t* priv = thiz != NULL ? (Priv_t*)thiz->priv_subclass[0] : NULL
#define DECL_PRIV1(thiz, priv) Priv_t* priv = thiz != NULL ? (Priv_t*)thiz->priv_subclass[1] : NULL

#define HALF(a)     ((a)>>1)
#define MIN(a, b)   (a) < (b) ? (a) : (b)
#define MAX(a, b)   (a) < (b) ? (b) : (a)
#define ABS(a)		(((a) > 0) ? (a) : (a) * -1)
#define SQUARE(a)	((a) * (a))
#define SQRT(a)	    (sqrt(a))


#define MASK_BITS(val32, index) (((val32) << ((index)%32)) & 0x80000000)

#endif/* end of SYS_TYPEDEF_H */

