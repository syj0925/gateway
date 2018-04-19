/********************************************************************************
**
** �ļ���:     sys_swconfig.h
** ��Ȩ����:   (c) 2013-2014 
** �ļ�����:   ��ģ����Ҫʵ�ָ�����ģ�鿪�ؿ���
**
*********************************************************************************
**             �޸���ʷ��¼
**===============================================================================
**| ����       | ����   |  �޸ļ�¼
**===============================================================================
**| 2016/03/16 | ���ѽ� |  ������һ�汾
*********************************************************************************/
#ifndef SYS_SWCONFIG_H
#define SYS_SWCONFIG_H           1

/*
*****************************************************************************************
*  ע: 1.���ļ�����.C�ļ�������Ҫ�õ��꿪�صط�
*      2.���ļ�����ŵ�.C����ˣ������ڱ��ļ�֮�϶���ĺ꿪�ؽ�ʧЧ�����±�����������
*      3.һ��Ҫ�����е�.C��û������ͷ�ļ�����Ϊ����ʱ���ᾯ��ģ�����������bug���꿪������
*        �Ĵ��벻�ᱻ�����ȥ(pp�����Ǳ������׳���)
*****************************************************************************************
*/



/*
*****************************************************************************************
*   �汾�Ŷ���
*****************************************************************************************
*/
#define DEVICE_TYPE              1
#define HARDWARE_VERSION_STR     "hard-v1.0"
#define SOFTWARE_VERSION_STR     "0000"

/*
*****************************************************************************************
* ������ģ��ı�����ƿ���
*****************************************************************************************
*/
#define EN_CPU_USAGE         0         /* CPUʹ����ͳ�� */
#define EN_UPDATE            1         /* Զ������ */

#define EN_ZIGBEE            0         /* ZIGBEE���� */
#define EN_WIFI              1         /* WIFI���� */
#define EN_ETHERNET          0         /* ��̫������ */
#define EN_YEELINK           0         /* YEELINK,��ǰ���뿪��EN_ETHERNET��wifi */

#define EN_DALI              0         /* DALI���� */

#define EN_NRF24L01          1         /* �������� */


#endif /* end of SYS_SWCONFIG_H */
