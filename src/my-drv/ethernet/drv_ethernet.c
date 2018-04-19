/********************************************************************************
**
** 文件名:     drv_ethernet.c
** 版权所有:   (c) 2013-2015 
** 文件描述:   以太网驱动模块
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**|    日期    |  作者  |  修改记录
**===============================================================================
**| 2015/06/24 | 苏友江 |  创建该文件
********************************************************************************/ 
#include "bsp.h"
#include "sys_includes.h"
#if EN_ETHERNET > 0

#if DBG_ETHERNET > 0
#define SYS_DEBUG          OS_DEBUG
#else
#define SYS_DEBUG(...)     do{}while(0)
#endif

/*
********************************************************************************
* 参数配置
********************************************************************************
*/


#define MAX_SOCK_NUM        3
#define SOCK_TCPC           0
#define SOCK_UDPC           1
#define SOCK_DHCP           2

#define MY_MAX_DHCP_RETRY	3

static INT8U gDATABUF[548];  //RIP_MSG_SIZE (236+OPT_SIZE)    /// Max size of @ref RIP_MSG

static BOOLEAN s_dhcping = 0;
static INT8U s_polltmr;
static Ip_para_t s_ip_para;

static wiz_NetInfo gWIZNETINFO = {  /* Default Network Configuration */
                             {0xc8, 0x9c, 0xdc, 0x33, 0x91, 0xf9},
                             {192, 168,  31, 120},
                             {255, 255, 255,   0},
                             {192, 168,  16,   1},
                             {  0,   0,  0,  0  },
                              NETINFO_DHCP      };
                             
static wiz_NetTimeout s_net_timeout = {
    3,
    2000
};

static INT16U any_port = 5004;
static INT8U  s_udp_destip[4];
static INT8U  s_udp_broadcast[4] = {255,255,255,255};
static INT8U  DIP[4]  = {224,1,1,1};
static INT8U  DHAR[6] = {0x01,0x00,0x5e,0x01,0x01,0x0b};
static INT16U s_dest_udp_port = 65000;
static INT16U s_group_port;

static INT8U ch_status[MAX_SOCK_NUM] = {0}; /** 0:close, 1:ready, 2:connected */
static BOOLEAN s_udp_is_recv = 0;
static INT32U s_waitTime = 0;
 
/*******************************************************************
** 函数名:      _w5500_reg_interface
** 函数描述:    First of all, Should register SPI callback functions implemented by user for accessing WIZCHIP
** 参数:        none
** 返回:        none
********************************************************************/
static void _w5500_reg_interface(void)
{
    /* Critical section callback */
    reg_wizchip_cris_cbfunc(spi_crisenter, spi_crisexit);                      /* 注册临界区函数 */
    /* Chip selection call back */
#if   _WIZCHIP_IO_MODE_ == _WIZCHIP_IO_MODE_SPI_VDM_
    reg_wizchip_cs_cbfunc(spi_cs_select, spi_cs_deselect);                     /* 注册SPI片选信号函数 */
#elif _WIZCHIP_IO_MODE_ == _WIZCHIP_IO_MODE_SPI_FDM_
    reg_wizchip_cs_cbfunc(spi_cs_select, spi_cs_deselect);                     /* CS must be tried with LOW. */
#else
   #if (_WIZCHIP_IO_MODE_ & _WIZCHIP_IO_MODE_SIP_) != _WIZCHIP_IO_MODE_SIP_
      #error "Unknown _WIZCHIP_IO_MODE_"
   #else
      reg_wizchip_cs_cbfunc(wizchip_select, wizchip_deselect);
   #endif
#endif
    /* SPI Read & Write callback function */
    reg_wizchip_spi_cbfunc(spi_read_byte, spi_write_byte);                     /* 注册读写函数 */
}

static void _network_init(void)
{
    INT8U tmpstr[6];

    ctlnetwork(CN_SET_NETINFO, (void*)&gWIZNETINFO);
    ctlnetwork(CN_GET_NETINFO, (void*)&gWIZNETINFO);
    ctlnetwork(CN_SET_TIMEOUT, (void*)&s_net_timeout);

    /* UDP主播设置 */
    setSn_DIPR(SOCK_UDPC, DIP);
    setSn_DHAR(SOCK_UDPC, DHAR);
    setSn_DPORT(SOCK_UDPC, s_dest_udp_port);

    /* Display Network Information */
    ctlwizchip(CW_GET_ID,(void*)tmpstr);

    if(gWIZNETINFO.dhcp == NETINFO_DHCP) {
        SYS_DEBUG("NET DHCP\n");
    } else {
        SYS_DEBUG("NET Static\n");
    }
    
    SYS_DEBUG("MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",gWIZNETINFO.mac[0],gWIZNETINFO.mac[1],gWIZNETINFO.mac[2],
          gWIZNETINFO.mac[3],gWIZNETINFO.mac[4],gWIZNETINFO.mac[5]);
    SYS_DEBUG("SIP: %d.%d.%d.%d\r\n", gWIZNETINFO.ip[0],gWIZNETINFO.ip[1],gWIZNETINFO.ip[2],gWIZNETINFO.ip[3]);
    SYS_DEBUG("GAR: %d.%d.%d.%d\r\n", gWIZNETINFO.gw[0],gWIZNETINFO.gw[1],gWIZNETINFO.gw[2],gWIZNETINFO.gw[3]);
    SYS_DEBUG("SUB: %d.%d.%d.%d\r\n", gWIZNETINFO.sn[0],gWIZNETINFO.sn[1],gWIZNETINFO.sn[2],gWIZNETINFO.sn[3]);
    SYS_DEBUG("DNS: %d.%d.%d.%d\r\n", gWIZNETINFO.dns[0],gWIZNETINFO.dns[1],gWIZNETINFO.dns[2],gWIZNETINFO.dns[3]);
    SYS_DEBUG("DIP: %d.%d.%d.%d\r\n", s_ip_para.server_ip[0],s_ip_para.server_ip[1],s_ip_para.server_ip[2],s_ip_para.server_ip[3]);
}

/*******************************************************************
** 函数名:      _my_ip_conflict
** 函数描述:    brief Call back for ip Conflict
** 参数:        none
** 返回:        none
********************************************************************/
static void _my_ip_conflict(void)
{
	SYS_DEBUG("CONFLICT IP from DHCP\r\n");
	//halt or reset or any...
	while(1); // this example is halt.
}

/*******************************************************************
** 函数名:      _my_ip_assign
** 函数描述:    brief Call back for ip assing & ip update from DHCP
** 参数:        none
** 返回:        none
********************************************************************/
static void _my_ip_assign(void)
{
   getIPfromDHCP(gWIZNETINFO.ip);
   getGWfromDHCP(gWIZNETINFO.gw);
   getSNfromDHCP(gWIZNETINFO.sn);
   getDNSfromDHCP(gWIZNETINFO.dns);
   gWIZNETINFO.dhcp = NETINFO_DHCP;
   /* Network initialization */
   _network_init();      // apply from dhcp
   SYS_DEBUG("DHCP LEASED TIME : %d Sec.\r\n", getDHCPLeasetime());
   s_dhcping = 0;
}

static void _socket_tcpc_poll(void)
{
    static INT32U presentTime = 0;
    
    switch (getSn_SR(SOCK_TCPC))
    {
        case SOCK_ESTABLISHED:                                                 /* if connection is established */
            if(ch_status[SOCK_TCPC] == 1) {
                SYS_DEBUG("<socket:%d : Connected>\r\n", SOCK_TCPC);
                ch_status[SOCK_TCPC] = 2;
            }
        break;
        
        case SOCK_CLOSE_WAIT:                                                  /* If the client request to close */
            SYS_DEBUG("<socket:%d : CLOSE_WAIT>\r\n", SOCK_TCPC);
            disconnect(SOCK_TCPC);
            ch_status[SOCK_TCPC] = 0;
            break;
            
        case SOCK_CLOSED:                                                      /* if a socket is closed */
        {
            if(g_run_sec - presentTime >= 3) {
            
                if(!ch_status[SOCK_TCPC])
                {
                    SYS_DEBUG("<socket:%d : TCP Client Started. port: %d>\r\n", SOCK_TCPC, s_ip_para.server_port);
                    ch_status[SOCK_TCPC] = 1;
                }
                if(socket(SOCK_TCPC, Sn_MR_TCP, any_port++, 0x00) != SOCK_TCPC)/* reinitialize the socket */
                {
                    SYS_DEBUG("<socket:%d : Fail to create socket.>\r\n", SOCK_TCPC);
                    ch_status[SOCK_TCPC] = 0;
                }

                presentTime = g_run_sec;
            }            
            break;
        }
        case SOCK_INIT:     /* if a socket is initiated */
        {
            if(g_run_sec - presentTime >= 3) {                            /* For TCP client's connection request delay : 3 sec */
                SYS_DEBUG("<socket:%d : connect socket.>\r\n", SOCK_TCPC);
                connect(SOCK_TCPC, s_ip_para.server_ip, s_ip_para.server_port);
                presentTime = g_run_sec;
            }
            break;
        }     
        default:
            break;
    }
}

static void _socket_udpc_poll(void)
{
    switch(getSn_SR(SOCK_UDPC))
    {
        case SOCK_UDP :

            break;
        case SOCK_CLOSED:
            SYS_DEBUG("<socket:%d:LBUStart>\r\n", SOCK_UDPC);
			socket(3, Sn_MR_UDP, 7000, 0x00);
            if (socket(SOCK_UDPC, Sn_MR_MULTI|Sn_MR_UDP, 6000, 0x00) != SOCK_UDPC) {
                SYS_DEBUG("<socket:%d:Opened fail!>\r\n",SOCK_UDPC);
            } else {
                SYS_DEBUG("<socket:%d:Opened, port [%d]>\r\n",SOCK_UDPC, s_dest_udp_port);
            }
            break;
        default :
            break;
    }
}

static void _socket_dhcp_poll(void)
{   
	static INT8U my_dhcp_retry = 0;
    
    switch(DHCP_run())
    {
    	case DHCP_IP_ASSIGN:
    	case DHCP_IP_CHANGED:
    		/* If this block empty, act with default_ip_assign & default_ip_update */
    		//
    		// This example calls _my_ip_assign in the two case.
    		//
    		// Add to ...
    		//
    		break;
    	case DHCP_IP_LEASED:
    		//
    		// TO DO YOUR NETWORK APPs.
    		//
    		break;
    	case DHCP_FAILED:
    		/* ===== Example pseudo code =====  */
    		// The below code can be replaced your code or omitted.
    		// if omitted, retry to process DHCP
    		my_dhcp_retry++;
    		if(my_dhcp_retry > MY_MAX_DHCP_RETRY)
    		{
    		    s_dhcping = 0;
    			SYS_DEBUG(">> DHCP %d Failed\r\n", my_dhcp_retry);
    			my_dhcp_retry = 0;
    			DHCP_stop();      // if restart, recall DHCP_init()
    			_network_init();   // apply the default static network and print out netinfo to serial
    		}
    		break;
    	default:
    		break;
    }
}

static void _poll_tmr(void *index)
{
    INT8U tmp;
    INT32U curTime, dhcp = 0;
    static BOOLEAN flag = 0;

    if (dhcp != g_run_sec) {
        dhcp = g_run_sec;
        DHCP_time_handler();
    }

    /* PHY link status check */
    if (ctlwizchip(CW_GET_PHYLINK, (void*)&tmp) == -1) {
        SYS_DEBUG("Unknown PHY Link stauts\r\n");
        return;
    } else {        
        if (tmp == PHY_LINK_OFF) {
            if (flag == 0) {
                flag = 1;
                close(SOCK_TCPC);
                ch_status[SOCK_TCPC] = 0;
                SYS_DEBUG("PHY Link stauts = PHY_LINK_OFF.\r\n");
            }
            return;
        } else if (tmp == PHY_LINK_ON) {
             if (flag == 1) {
                flag = 0;
                SYS_DEBUG("PHY Link stauts = PHY_LINK_ON.\r\n");
             }
        }
    }

    if (s_dhcping == 0) {
        _socket_tcpc_poll();
        _socket_udpc_poll();
    } else {
        _socket_dhcp_poll();
    }

    if (s_udp_is_recv == 1) {
        curTime = g_run_sec;
        
        if (s_waitTime == 0) {
        	s_waitTime = g_run_sec;
        }

		if (curTime - s_waitTime > 5) {
			s_udp_is_recv = 0;
			s_waitTime = 0;
		}        
    }
}

/*******************************************************************
** 函数名:      drv_ethernet_init
** 函数描述:    Intialize the network information to be used in WIZCHIP
** 参数:        none
** 返回:        none
********************************************************************/
void drv_ethernet_init(void)
{
    INT8U tmp;
    INT8U memsize[2][8] = {{2,2,2,2,2,2,2,2}, {2,2,2,2,2,2,2,2}};    

    memset(ch_status, 0, sizeof(ch_status));
    spi_configuration();
    reset_w5500();
    _w5500_reg_interface();

    /* WIZCHIP SOCKET Buffer initialize */
    if (ctlwizchip(CW_INIT_WIZCHIP,(void*)memsize) == -1) {
         SYS_DEBUG("WIZCHIP Initialized fail.\r\n");
         while(1);
    }

    /* PHY link status check */
    if (ctlwizchip(CW_GET_PHYLINK, (void*)&tmp) == -1) {
        SYS_DEBUG("Unknown PHY Link stauts.\r\n");
    } else {
        if (tmp == PHY_LINK_OFF) {
            SYS_DEBUG("PHY Link stauts = PHY_LINK_OFF.\r\n");
        } else if (tmp == PHY_LINK_ON) {
            SYS_DEBUG("PHY Link stauts = PHY_LINK_ON.\r\n");
        }
    }

    if (!public_para_manager_read_by_id(IP_PARA_, (INT8U*)&s_ip_para, sizeof(Ip_para_t))) {        
        s_ip_para.dhcp = 2;
    }

    memcpy(gWIZNETINFO.mac, s_ip_para.mac, sizeof(gWIZNETINFO.mac));
    memcpy(gWIZNETINFO.ip,  s_ip_para.ip,  sizeof(gWIZNETINFO.ip));
    memcpy(gWIZNETINFO.sn,  s_ip_para.sn,  sizeof(gWIZNETINFO.sn));
    memcpy(gWIZNETINFO.gw,  s_ip_para.gw,  sizeof(gWIZNETINFO.gw));
    memcpy(gWIZNETINFO.dns, s_ip_para.dns, sizeof(gWIZNETINFO.dns));
    gWIZNETINFO.dhcp = (dhcp_mode)s_ip_para.dhcp;
    
    /* Network initialization */
    if (s_ip_para.dhcp == 1) {
        s_dhcping = 0;
        _network_init();
    } else {
        s_dhcping = 1;
    
        setSHAR(gWIZNETINFO.mac);
        DHCP_init(SOCK_DHCP, gDATABUF);
        // if you want defiffent action instead defalut ip assign,update, conflict,
        // if cbfunc == 0, act as default.
        reg_dhcp_cbfunc(_my_ip_assign, _my_ip_assign, _my_ip_conflict);
    }

    s_polltmr = os_timer_create(0, _poll_tmr);
    os_timer_start(s_polltmr, MILTICK);
}

INT32S drv_ethernet_write(INT8U* pdata, INT16U datalen)
{
    if (s_udp_is_recv == 1) {
        //sendto(SOCK_UDPC, pdata, datalen, s_udp_destip, s_group_port);
		sendto(3, pdata, datalen, s_udp_broadcast, 60000);
    } else {
        send(SOCK_TCPC, pdata, datalen);
    }
    return datalen;
}

INT32S drv_ethernet_read(INT8U* pdata, INT16U datalen)
{
	INT32S ret = 0;
    
    if ((ret = getSn_RX_RSR(SOCK_TCPC)) > 0) {
        if (ret > datalen) {
            ret = datalen;
        }
        ret = recv(SOCK_TCPC, pdata, ret);                                     /* read the received data */
        return ret;        
    }

    if ((ret = getSn_RX_RSR(SOCK_UDPC)) > 0) {
        if (ret > datalen) {
            ret = datalen;
        }
        
        s_waitTime    = 0;
        s_udp_is_recv = 1;
        ret = recvfrom(SOCK_UDPC, pdata, ret, s_udp_destip, (INT16U*)&s_group_port); /* read the received data */
    }

    return ret;      
}

BOOLEAN drv_ethernet_is_connect(void)
{
    if (ch_status[SOCK_TCPC] == 2) {
        return TRUE;
    } else {
        return FALSE;
    }
}

#ifdef ETHERNET_TEST

#define SOCK_TCPS        0
#define SOCK_UDPS        1
#define DATA_BUF_SIZE   512

INT8U gDATABUF[DATA_BUF_SIZE];

/**
  * @brief  Loopback Test Example Code using ioLibrary_BSD	
  * @retval None
  */
static INT32S loopback_tcps(INT8U sn, INT8U* buf, INT16U port)
{
   INT32S ret;
   INT16U size = 0, sentsize=0;
   switch(getSn_SR(sn))
   {
      case SOCK_ESTABLISHED :
         if(getSn_IR(sn) & Sn_IR_CON)
         {
            SYS_DEBUG("%d:Connected\r\n",sn);
            setSn_IR(sn,Sn_IR_CON);
         }
         if((size = getSn_RX_RSR(sn)) > 0)
         {
            if(size > DATA_BUF_SIZE) size = DATA_BUF_SIZE;
            ret = recv(sn,buf,size);
            if(ret <= 0) return ret;
            sentsize = 0;
            while(size != sentsize)
            {
               ret = send(sn,buf+sentsize,size-sentsize);
               if(ret < 0)
               {
                  close(sn);
                  return ret;
               }
               sentsize += ret;                                                /* Don't care SOCKERR_BUSY, because it is zero. */
            }
         }
         break;
      case SOCK_CLOSE_WAIT :
         SYS_DEBUG("%d:CloseWait\r\n",sn);
         if((ret=disconnect(sn)) != SOCK_OK) return ret;
         SYS_DEBUG("%d:Closed\r\n",sn);
         break;
      case SOCK_INIT :
    	  SYS_DEBUG("%d:Listen, port [%d]\r\n",sn, port);
         if( (ret = listen(sn)) != SOCK_OK) return ret;
         break;
      case SOCK_CLOSED:
         SYS_DEBUG("%d:LBTStart\r\n",sn);
         if((ret=socket(sn,Sn_MR_TCP,port,0x00)) != sn)
            return ret;
         SYS_DEBUG("%d:Opened\r\n",sn);
         break;
      default:
         break;
   }
   return 1;
}

/**
  * @brief  Loopback Test Example Code using ioLibrary_BSD	
  * @retval None
  */
static INT32S loopback_udps(INT8U sn, INT8U* buf, INT16U port)
{
   INT32S  ret;
   INT16U size, sentsize;
   INT8U  destip[4];
   INT16U destport;
                                                                               /* INT8U  packinfo = 0; */
   switch(getSn_SR(sn))
   {
      case SOCK_UDP :
         if((size = getSn_RX_RSR(sn)) > 0)
         {
            if(size > DATA_BUF_SIZE) size = DATA_BUF_SIZE;
            ret = recvfrom(sn, buf, size, destip, (INT16U*)&destport);
            if(ret <= 0)
            {
               SYS_DEBUG("%d: recvfrom error. %ld\r\n",sn,ret);
               return ret;
            }
            size = (INT16U) ret;
            sentsize = 0;
            while(sentsize != size)
            {
               ret = sendto(sn, buf+sentsize, size-sentsize, destip, destport);
               if(ret < 0)
               {
                  SYS_DEBUG("%d: sendto error. %ld\r\n",sn,ret);
                  return ret;
               }
               sentsize += ret;                                                /* Don't care SOCKERR_BUSY, because it is zero. */
            }
         }
         break;
      case SOCK_CLOSED:
         SYS_DEBUG("%d:LBUStart\r\n",sn);
         if((ret=socket(sn,Sn_MR_UDP,port,0x00)) != sn)
            return ret;
         SYS_DEBUG("%d:Opened, port [%d]\r\n",sn, port);
         break;
      default :
         break;
   }
   return 1;
}

/*******************************************************************
** 函数名:      test_loopback
** 函数描述:    Loopback Test
** 参数:        none
** 返回:        none
********************************************************************/
void test_loopback(void)
{
	INT32S ret = 0;
   
	/* Loopback Test */
	/* TCP server loopback test */
	if( (ret = loopback_tcps(SOCK_TCPS, gDATABUF, 5000)) < 0) {
		SYS_DEBUG("SOCKET ERROR : %ld\r\n", ret);
	}
    
    /* UDP server loopback test */
	if( (ret = loopback_udps(SOCK_UDPS, gDATABUF, 3000)) < 0) {
		SYS_DEBUG("SOCKET ERROR : %ld\r\n", ret);
	}
}

#endif
#endif /* end of EN_ETHERNET */

