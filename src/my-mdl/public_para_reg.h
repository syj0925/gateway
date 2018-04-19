/********************************************************************************
**
** 文件名:     public_para_reg.h
** 版权所有:   (c) 2013-2015 厦门智联信通物联网科技有限公司
** 文件描述:   实现公共参数文件存储驱动注册信息表管理
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**|    日期    |  作者  |  修改记录
**===============================================================================
**| 2015/08/26 | 苏友江 |  创建该文件
********************************************************************************/
#ifndef PUBLIC_PARA_REG_H
#define PUBLIC_PARA_REG_H

#include "public_para_typedef.h"

/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
typedef enum {
    PP_BK,    
    PP_NBK,
    PP_BK_MAX
} PP_BK_E;

/* 公共参数信息结构体 */
typedef struct {
    INT8U        type;                    /* 公共参数文件类型 */
    INT8U        id;                      /* 公共参数统一编号 */
    INT16U       rec_size;                /* 参数长度 */
    INT8U        bk;                      /* 备份属性 */
    void const  *i_ptr;                   /* 默认参数指针 */
} PP_REG_T;

/* 公共参数类结构体 */
typedef struct {
    INT8U           type;                 /* 公共参数文件类型 */
    INT16U          dly;                  /* 延时存储到文件时间，单位：秒 */
    char const     *filename;             /* 公共参数文件文件名,以'\0'为结束符 */
    INT32U          memlen;               /* 映射内存大小 */
    INT8U          *memptr;               /* 映射内存 */
    PP_REG_T const *preg;                 /* 各个公共参数类注册信息表 */
    INT8U           nreg;                 /* 各个公共参数类的参数的个数 */
} PP_CLASS_T;

typedef struct {
    INT8U chksum[2];
} PP_HEAD_T;

/*
********************************************************************************
* 定义所有数据库文件类统一编号
********************************************************************************
*/
#ifdef BEGIN_PP_DEF
#undef BEGIN_PP_DEF
#endif

#ifdef PP_DEF
#undef PP_DEF
#endif

#ifdef END_PP_DEF
#undef END_PP_DEF
#endif

#define BEGIN_PP_DEF(_PP_TYPE_, _DLY_, _DESC_)  \
               _PP_TYPE_##_ENUM,
               
#define PP_DEF(_PP_TYPE_, _PP_ID_, _LEN_, _BAK_, _I_PTR_)
#define END_PP_DEF(_PP_TYPE_)

typedef enum {
    #include "public_para_reg.def"
    PP_CLASS_ID_MAX
} PP_CLASS_ID_E;

/* 定义类统一编号 */
#ifdef BEGIN_PP_DEF
#undef BEGIN_PP_DEF
#endif

#ifdef PP_DEF
#undef PP_DEF
#endif

#ifdef END_PP_DEF
#undef END_PP_DEF
#endif

#ifdef GLOBALS_PP_REG
#define BEGIN_PP_DEF(_PP_TYPE_, _DLY_, _DESC_)  \
               INT8U const _PP_TYPE_ = _PP_TYPE_##_ENUM;
               
#define PP_DEF(_PP_TYPE_, _PP_ID_, _LEN_, _BAK_, _I_PTR_)
#define END_PP_DEF(_PP_TYPE_)

#include "public_para_reg.def"
#else
#define BEGIN_PP_DEF(_PP_TYPE_, _DLY_, _DESC_)  \
               extern INT8U const _PP_TYPE_;
               
#define PP_DEF(_PP_TYPE_, _PP_ID_, _LEN_, _BAK_, _I_PTR_)
#define END_PP_DEF(_PP_TYPE_)

#include "public_para_reg.def"
#endif

/*
********************************************************************************
* 定义所有数据库文件统一编号
********************************************************************************
*/
#ifdef BEGIN_PP_DEF
#undef BEGIN_PP_DEF
#endif

#ifdef PP_DEF
#undef PP_DEF
#endif

#ifdef END_PP_DEF
#undef END_PP_DEF
#endif

#define BEGIN_PP_DEF(_PP_TYPE_, _DLY_, _DESC_)
#define PP_DEF(_PP_TYPE_, _PP_ID_, _LEN_, _BAK_, _I_PTR_) \
               _PP_ID_##_ENUM,
                  
#define END_PP_DEF(_PP_TYPE_)

typedef enum {
    #include "public_para_reg.def"
    PP_ID_MAX
} PP_ID_E;

/* 定义统一编号 */
#ifdef BEGIN_PP_DEF
#undef BEGIN_PP_DEF
#endif

#ifdef PP_DEF
#undef PP_DEF
#endif

#ifdef END_PP_DEF
#undef END_PP_DEF
#endif

#ifdef GLOBALS_PP_REG
#define BEGIN_PP_DEF(_PP_TYPE_, _DLY_, _DESC_)
#define PP_DEF(_PP_TYPE_, _PP_ID_, _LEN_, _BAK_, _I_PTR_) \
               INT8U const _PP_ID_ = _PP_ID_##_ENUM;
                  
#define END_PP_DEF(_PP_TYPE_)

#include "public_para_reg.def"
#else

#define BEGIN_PP_DEF(_PP_TYPE_, _DLY_, _DESC_)
#define PP_DEF(_PP_TYPE_, _PP_ID_, _LEN_, _BAK_, _I_PTR_) \
               extern INT8U const _PP_ID_;
                  
#define END_PP_DEF(_PP_TYPE_)

#include "public_para_reg.def"
#endif

/*******************************************************************
** 函数名:     public_para_reg_get_class_info
** 函数描述:   获取对应公共参数类的注册信息
** 参数:       [in] nclass:统一编号的类编号，见PP_CLASS_ID_E
** 返回:       成功返回注册表指针，失败返回0
********************************************************************/
PP_CLASS_T const *public_para_reg_get_class_info(INT8U nclass);

/*******************************************************************
*   函数名:    public_para_reg_get_class_max
*   功能描述:  获取已注册的公共参数类个数
*　 参数:      无
*   返回值:    类个数
*******************************************************************/ 
INT8U public_para_reg_get_class_max(void);

/*******************************************************************
** 函数名:     public_para_reg_get_item_info
** 函数描述:   获取对应公共参数的注册信息
** 参数:       [in] id:统一编号的参数编号，见PP_ID_E
** 返回:       成功返回注册表指针，失败返回0
********************************************************************/
PP_REG_T const *public_para_reg_get_item_info(INT8U id);

/*******************************************************************
*   函数名:    public_para_reg_get_item_max
*   功能描述:  获取已注册的公共参数个数
*　 参数:      无
*   返回值:    个数
*******************************************************************/
INT8U public_para_reg_get_item_max(void);

#endif



