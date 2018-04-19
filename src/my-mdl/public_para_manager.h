/********************************************************************************
**
** 文件名:     public_para_manager.h
** 版权所有:   (c) 2013-2015 
** 文件描述:   实现公共参数文件存储驱动管理
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**|    日期    |  作者  |  修改记录
**===============================================================================
**| 2015/08/25 | 苏友江 |  创建该文件
********************************************************************************/
#ifndef PUBLIC_PARA_MANAGER_H
#define PUBLIC_PARA_MANAGER_H   1

#include "public_para_reg.h"
#include "public_para_typedef.h"

/*
********************************************************************************
* 定义模块配置参数
********************************************************************************
*/

// pubpara change reason
typedef enum {
    PP_REASON_STORE,                   /* 存储 */
    PP_REASON_SIMBAK,                  /* 取出备份参数 */
    PP_REASON_RESET,                   /* 恢复出厂设置 */
    PP_REASON_MAX
} PP_REASON_E;

#define MAX_PP_NUM           20        /* 最大参数个数 */
#define MAX_PP_CLASS_NUM     3         /* 最大数据区个数 */

#define MAX_PP_INFORM        20        /* 最大注册参数变化通知函数个数 */
#define MAX_PP_FILENAME_LEN  6



/*******************************************************************
** 函数名:      public_para_manager_init
** 函数描述:    公共参数存储驱动初始化
** 参数:        无
** 返回:        无
********************************************************************/
void public_para_manager_init(void);

/*******************************************************************
** 函数名:     public_para_manager_del_by_class
** 函数描述:   删除指定参数区的参数
** 参数:       [in] cls：参数区,见PP_CLASS_ID_E
** 返回:       成功返回true，失败返回false
********************************************************************/
//#define DAL_PP_DelParaByClass(cls)    PP_DelParaByClass(0x55, cls)
BOOLEAN public_para_manager_del_by_class(INT8U cls);

/*******************************************************************
** 函数名:     public_para_manager_del_all
** 函数描述:   删除所有参数区的参数,程序可以继续正常运行
** 参数:       无
** 返回:       成功返回true，失败返回false
********************************************************************/
//#define DAL_PP_DelAllPara()    PP_DelAllPara(0x55)
BOOLEAN public_para_manager_del_all(void);

/*******************************************************************
** 函数名:     public_para_manager_del_all_and_rst
** 函数描述:   删除所有参数区的参数,必须复位重启设备后才能继续存储
** 参数:       无
** 返回:       无
********************************************************************/
//#define DAL_PP_DelAllParaAndRst()    PP_DelAllParaAndRst(0x55)
void public_para_manager_del_all_and_rst(void);

/*******************************************************************
** 函数名:      public_para_manager_clear_by_id
** 函数描述:    清除PP参数
** 参数:        [in] id:  参数编号，见PP_ID_E
** 返回:        有效返回true，无效返回false
********************************************************************/
BOOLEAN public_para_manager_clear_by_id(INT8U id);

/*******************************************************************
** 函数名:      public_para_manager_read_by_id
** 函数描述:    读取PP参数，判断PP有效性
** 参数:        [in] id:  参数编号，见PP_ID_E
**              [in] dptr:输出缓存
**              [in] rlen:缓存长度
** 返回:        有效返回true，无效返回false
********************************************************************/
BOOLEAN public_para_manager_read_by_id(INT8U id, INT8U *dptr, INT16U rlen);

/*******************************************************************
** 函数名:      public_para_manager_store_by_id
** 函数描述:    存储PP参数，延时2秒存储到flash
** 参数:        [in] id:  参数编号，见PP_ID_E
**              [in] sptr:输入缓存
**              [in] slen:缓存长度
** 返回:        成功返回true，失败返回false
********************************************************************/
BOOLEAN public_para_manager_store_by_id(INT8U id, INT8U *sptr, INT16U slen);

/*******************************************************************
** 函数名:      public_para_manager_store_instant_by_id
** 函数描述:    存储PP参数,立即存储到flash中
** 参数:        [in] id:  参数编号，见PP_ID_E
**              [in] sptr:输入缓存
**              [in] slen:缓存长度
** 返回:        成功返回true，失败返回false
********************************************************************/
BOOLEAN public_para_manager_store_instant_by_id(INT8U id, INT8U *sptr, INT16U slen);

/*******************************************************************
** 函数名:      public_para_manager_check_valid_by_id
** 函数描述:    判断PP参数有效性
** 参数:        [in] id:  参数编号，见PP_ID_E
** 返回:        有效返回true，无效返回false
********************************************************************/
BOOLEAN public_para_manager_check_valid_by_id(INT8U id);

/*******************************************************************
** 函数名:      public_para_manager_get_by_id
** 函数描述:    获取PP参数，不判断PP参数的有效性，直接获取
** 参数:        [in] id:  参数编号，见PP_ID_E
**              [in] dptr:输出缓存
**              [in] rlen:缓存长度
** 返回:        成功返回true，失败返回false
********************************************************************/
BOOLEAN public_para_manager_get_by_id(INT8U id, INT8U *dptr, INT16U rlen);

/*******************************************************************
** 函数名:      public_para_manager_reg_change_informer
** 函数描述:    注册参数变化通知函数的函数
** 参数:        [in] id:  参数编号，见PP_ID_E
**              [in] fp:  回调函数
** 返回:        成功返回true，失败返回false
********************************************************************/
BOOLEAN public_para_manager_reg_change_informer(INT8U id, void (*fp)(INT8U reason));

/*******************************************************************
** 函数名:      public_para_manager_inform_change_by_id
** 函数描述:    参数变化通知处理
** 参数:        [in] id:      参数编号，见PP_ID_E
**              [in] reason:  变化原因
** 返回:        无
********************************************************************/
void public_para_manager_inform_change_by_id(INT16U id, INT8U reason);

/*******************************************************************
** 函数名:     public_para_manager_reset_inform
** 函数描述:   复位回调接口
** 参数:       [in] resettype：复位类型
**             [in] filename： 文件名
**             [in] line：     出错行号
**             [in] errid：    错误ID
** 返回:       无
********************************************************************/
void public_para_manager_reset_inform(INT8U resettype, INT8U *filename, INT32U line, INT32U errid);

#endif

