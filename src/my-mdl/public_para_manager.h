/********************************************************************************
**
** �ļ���:     public_para_manager.h
** ��Ȩ����:   (c) 2013-2015 
** �ļ�����:   ʵ�ֹ��������ļ��洢��������
**
*********************************************************************************
**             �޸���ʷ��¼
**===============================================================================
**|    ����    |  ����  |  �޸ļ�¼
**===============================================================================
**| 2015/08/25 | ���ѽ� |  �������ļ�
********************************************************************************/
#ifndef PUBLIC_PARA_MANAGER_H
#define PUBLIC_PARA_MANAGER_H   1

#include "public_para_reg.h"
#include "public_para_typedef.h"

/*
********************************************************************************
* ����ģ�����ò���
********************************************************************************
*/

// pubpara change reason
typedef enum {
    PP_REASON_STORE,                   /* �洢 */
    PP_REASON_SIMBAK,                  /* ȡ�����ݲ��� */
    PP_REASON_RESET,                   /* �ָ��������� */
    PP_REASON_MAX
} PP_REASON_E;

#define MAX_PP_NUM           20        /* ���������� */
#define MAX_PP_CLASS_NUM     3         /* ������������� */

#define MAX_PP_INFORM        20        /* ���ע������仯֪ͨ�������� */
#define MAX_PP_FILENAME_LEN  6



/*******************************************************************
** ������:      public_para_manager_init
** ��������:    ���������洢������ʼ��
** ����:        ��
** ����:        ��
********************************************************************/
void public_para_manager_init(void);

/*******************************************************************
** ������:     public_para_manager_del_by_class
** ��������:   ɾ��ָ���������Ĳ���
** ����:       [in] cls��������,��PP_CLASS_ID_E
** ����:       �ɹ�����true��ʧ�ܷ���false
********************************************************************/
//#define DAL_PP_DelParaByClass(cls)    PP_DelParaByClass(0x55, cls)
BOOLEAN public_para_manager_del_by_class(INT8U cls);

/*******************************************************************
** ������:     public_para_manager_del_all
** ��������:   ɾ�����в������Ĳ���,������Լ�����������
** ����:       ��
** ����:       �ɹ�����true��ʧ�ܷ���false
********************************************************************/
//#define DAL_PP_DelAllPara()    PP_DelAllPara(0x55)
BOOLEAN public_para_manager_del_all(void);

/*******************************************************************
** ������:     public_para_manager_del_all_and_rst
** ��������:   ɾ�����в������Ĳ���,���븴λ�����豸����ܼ����洢
** ����:       ��
** ����:       ��
********************************************************************/
//#define DAL_PP_DelAllParaAndRst()    PP_DelAllParaAndRst(0x55)
void public_para_manager_del_all_and_rst(void);

/*******************************************************************
** ������:      public_para_manager_clear_by_id
** ��������:    ���PP����
** ����:        [in] id:  ������ţ���PP_ID_E
** ����:        ��Ч����true����Ч����false
********************************************************************/
BOOLEAN public_para_manager_clear_by_id(INT8U id);

/*******************************************************************
** ������:      public_para_manager_read_by_id
** ��������:    ��ȡPP�������ж�PP��Ч��
** ����:        [in] id:  ������ţ���PP_ID_E
**              [in] dptr:�������
**              [in] rlen:���泤��
** ����:        ��Ч����true����Ч����false
********************************************************************/
BOOLEAN public_para_manager_read_by_id(INT8U id, INT8U *dptr, INT16U rlen);

/*******************************************************************
** ������:      public_para_manager_store_by_id
** ��������:    �洢PP��������ʱ2��洢��flash
** ����:        [in] id:  ������ţ���PP_ID_E
**              [in] sptr:���뻺��
**              [in] slen:���泤��
** ����:        �ɹ�����true��ʧ�ܷ���false
********************************************************************/
BOOLEAN public_para_manager_store_by_id(INT8U id, INT8U *sptr, INT16U slen);

/*******************************************************************
** ������:      public_para_manager_store_instant_by_id
** ��������:    �洢PP����,�����洢��flash��
** ����:        [in] id:  ������ţ���PP_ID_E
**              [in] sptr:���뻺��
**              [in] slen:���泤��
** ����:        �ɹ�����true��ʧ�ܷ���false
********************************************************************/
BOOLEAN public_para_manager_store_instant_by_id(INT8U id, INT8U *sptr, INT16U slen);

/*******************************************************************
** ������:      public_para_manager_check_valid_by_id
** ��������:    �ж�PP������Ч��
** ����:        [in] id:  ������ţ���PP_ID_E
** ����:        ��Ч����true����Ч����false
********************************************************************/
BOOLEAN public_para_manager_check_valid_by_id(INT8U id);

/*******************************************************************
** ������:      public_para_manager_get_by_id
** ��������:    ��ȡPP���������ж�PP��������Ч�ԣ�ֱ�ӻ�ȡ
** ����:        [in] id:  ������ţ���PP_ID_E
**              [in] dptr:�������
**              [in] rlen:���泤��
** ����:        �ɹ�����true��ʧ�ܷ���false
********************************************************************/
BOOLEAN public_para_manager_get_by_id(INT8U id, INT8U *dptr, INT16U rlen);

/*******************************************************************
** ������:      public_para_manager_reg_change_informer
** ��������:    ע������仯֪ͨ�����ĺ���
** ����:        [in] id:  ������ţ���PP_ID_E
**              [in] fp:  �ص�����
** ����:        �ɹ�����true��ʧ�ܷ���false
********************************************************************/
BOOLEAN public_para_manager_reg_change_informer(INT8U id, void (*fp)(INT8U reason));

/*******************************************************************
** ������:      public_para_manager_inform_change_by_id
** ��������:    �����仯֪ͨ����
** ����:        [in] id:      ������ţ���PP_ID_E
**              [in] reason:  �仯ԭ��
** ����:        ��
********************************************************************/
void public_para_manager_inform_change_by_id(INT16U id, INT8U reason);

/*******************************************************************
** ������:     public_para_manager_reset_inform
** ��������:   ��λ�ص��ӿ�
** ����:       [in] resettype����λ����
**             [in] filename�� �ļ���
**             [in] line��     �����к�
**             [in] errid��    ����ID
** ����:       ��
********************************************************************/
void public_para_manager_reset_inform(INT8U resettype, INT8U *filename, INT32U line, INT32U errid);

#endif
