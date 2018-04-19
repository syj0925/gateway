/********************************************************************************
**
** 文件名:     drv_dali.c
** 版权所有:   (c) 2013-2016 厦门智能窗户电子
** 文件描述:   1
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**|    日期    |  作者  |  修改记录
**===============================================================================
**| 2016/3/17 | 苏友江 |  创建该文件
********************************************************************************/
/********************************************************************************
**
** 文件名:     drv_dali.c
** 版权所有:   (c) 2013-2016 
** 文件描述:   DALI调光驱动
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**|    日期    |  作者  |  修改记录
**===============================================================================
**| 2016/03/16 | 苏友江 |  创建该文件
********************************************************************************/
#include "bsp.h"
#include "sys_includes.h"
#include "drv_dali.h"
#if EN_DALI > 0

#if DBG_DALI > 0
#define SYS_DEBUG          OS_DEBUG
#else
#define SYS_DEBUG(...)     do{}while(0)
#endif


typedef struct {
	INT8U address;
	INT8U dataByte;
	INT8U bit_count;
	INT8U tick_count;
	INT8U former_val;
	dali_action_enum_type dali_flag;	
} dali_protocol_type;

#ifdef DALI_HOST
static INT32U dali_request = 0;	
#endif

#ifdef DALI_SLAVE
static INT8U answer = 0;	
#endif

static INT8U flag_exi = 0; 
static INT8U s_dalitmr;

static dali_protocol_type s_dali_controler;

#define DALI_TX_CLR()    GPIO_ResetBits(GPIOA, GPIO_Pin_4)
#define DALI_TX_SET()    GPIO_SetBits(GPIOA, GPIO_Pin_4)
#define DALI_TX_GETBIT() GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_4)
#define DALI_RX_GETBIT() GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_5)

void TIM6_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM6, TIM_IT_Update) == SET) {
		TIM_ClearITPendingBit(TIM6, TIM_IT_Update);
		if (s_dali_controler.dali_flag == DALI_RECEIVING_DATA) {
			dali_receive_tick();
		} else if (s_dali_controler.dali_flag == DALI_SENDING_DATA) {
			dali_send_tick();
		}
	}
}

/**
  * @brief  This function handles External lines 9 to 5 interrupt request.
  * @param  None
  * @retval None
  */
void EXTI9_5_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line7) != RESET) {
        /* Clear the EXTI line 7 pending bit */
        EXTI_ClearITPendingBit(EXTI_Line7);
        dali_receive_data();
    }
}

/*******************************************************************
** 函数名:      _set_dali_out
** 函数描述:    Set value to the DALIOUT pin
** 参数:        [in] pin_value
** 返回:        none
********************************************************************/
static void _set_dali_out(INT8U pin_value)
{
#ifdef DALI_LEVEL_INVERT
    if (pin_value) {
        DALI_TX_CLR();
    } else {
        DALI_TX_SET();
    }
#else
    if (pin_value) {
        DALI_TX_SET();
    } else {
        DALI_TX_CLR();
    }
#endif
}

/*******************************************************************
** 函数名:      _get_dali_out
** 函数描述:    gets state of the DALIOUT pin
** 参数:        none
** 返回:        INT8U
********************************************************************/
static INT8U _get_dali_out(void)
{
#ifdef DALI_LEVEL_INVERT
    if (DALI_TX_GETBIT()) {
        return FALSE;
    } else {
        return TRUE;
    }
#else
    if (DALI_TX_GETBIT()) {
        return TRUE;
    } else {
        return FALSE;
    }
#endif
}	

/*******************************************************************
** 函数名:      _get_dali_in
** 函数描述:    gets state of the DALIIN pin
** 参数:        none
** 返回:        INT8U
********************************************************************/
static INT8U _get_dali_in(void) 
{
#ifdef DALI_LEVEL_INVERT
    if (DALI_RX_GETBIT()) {
        return FALSE;
    } else {
        return TRUE;
    }
#else
    if (DALI_RX_GETBIT()) {
        return TRUE;
    } else {
        return FALSE;
    }
#endif
}

static void _cmp_address(INT32U addrIn)
{	
    INT8U addrT = 0;
    
    addrT = (addrIn&0xff0000) >> 16;	
    dali_send_data(DALI_CMP_ADDR_H|addrT, TRUE);
    addrT = (addrIn&0xff00) >> 8;
    dali_send_data(DALI_CMP_ADDR_M|addrT, TRUE);
    addrT = (addrIn&0xff);
    dali_send_data(DALI_CMP_ADDR_L|addrT, TRUE);
    dali_send_data(DALI_CMP_CMD, TRUE);
}

static void _dali_tmr(void *index)
{
	dali_auto_address();
}

void drv_dali_init(void)
{
    memset(&s_dali_controler, 0, sizeof(s_dali_controler));

    s_dalitmr = os_timer_create(0, _dali_tmr);
    os_timer_start(s_dalitmr, SECOND);
}

//void dali_send_data(INT8U *pt)
/*******************************************************************
** 函数名:      dali_send_data
** 函数描述:    Send answer to the controller device
** 参数:        [in] sendCmd
**              [in] isWaitRecv
** 返回:        none
********************************************************************/
void dali_send_data(INT16U sendCmd, INT8U isWaitRecv)
{
#ifdef DALI_HOST
    dali_request = sendCmd;
    SYS_DEBUG("dali_send:%x\r\n", dali_request);
#endif

#ifdef DALI_SLAVE
    answer = pt[0];
#endif

    s_dali_controler.bit_count  = 0;
    s_dali_controler.tick_count = 0;
    s_dali_controler.dataByte   = 0;

    /* disable external interrupt - no incoming data now */
    bsp_ext9_5_deconfig();

    s_dali_controler.dali_flag = DALI_SENDING_DATA;

    if (isWaitRecv == TRUE) {	
#if 0   /* 加入延时 */
        TaskSleepMs(40);
#endif /* if 0. 2016-3-17 下午 9:28:31 syj */
        SYS_DEBUG("dali recv:%x\n", s_dali_controler.dataByte);
    }
    //TIM4->CR1 |= TIM4_CR1_CEN;
}

/*******************************************************************
** 函数名:      dali_send_tick
** 函数描述:    DALI glbRtuPara physical layer for slave device
** 参数:        none
** 返回:        none
********************************************************************/
void dali_send_tick(void)
{
    static INT8U bit_value = 0;
    
#ifdef DALI_HOST
    INT8U tickOfStopBit = (2*8+1)*8+32;//(dataNum*8+1)*8+32
    
    //access to the routine just every 4 ticks = every half bit
    if ((s_dali_controler.tick_count & 0x03) == 0) {
        if (s_dali_controler.tick_count <tickOfStopBit) {
            // 0--[delay]settling time between forward and backward frame
            /* 延时3.3ms，但其实是没有必要，因为终端是受控设备 */
            if (s_dali_controler.tick_count < 32) {
                s_dali_controler.tick_count++;
                return;
            }

            // 1--[start_a]start of the start bit
            // 32 ticks = 8*Te time = delay between forward and backward message frame (1*Te time must be added as half of stop bit)
            if (s_dali_controler.tick_count == 32) {
                _set_dali_out(FALSE);
                s_dali_controler.tick_count++;
                return;
            }

            // 1.1--[start_b] edge of the start bit
            if (s_dali_controler.tick_count == 36) {
                _set_dali_out(TRUE);
                s_dali_controler.tick_count++;
                return;
            }

            // 2--[data]bit value (edge) selection
            bit_value = ((dali_request>> (15-s_dali_controler.bit_count)) & 0x01);
            // Every half bit -> Manchester coding
            // 2.1--[data_a]
            if (!((s_dali_controler.tick_count - 32) & 0x0007)) { // div by 8
                /* former value of bit = new value of bit */
                if (_get_dali_out() == bit_value) {
                    _set_dali_out((1-bit_value));
                }
            }

            // 2.2--[data_b]Generate edge for actual bit
            if (!((s_dali_controler.tick_count - 36) & 0x0007)) {
                _set_dali_out(bit_value);
                s_dali_controler.bit_count++;
            }
        } else { // 3--[end_a]end of data byte, start of stop bits
            if (s_dali_controler.tick_count == tickOfStopBit) {
                _set_dali_out(TRUE); // start of stop bit
            }

            // 3.3--[end_b]end of stop bits, no settling time
            if (s_dali_controler.tick_count == tickOfStopBit+16) {
                s_dali_controler.dali_flag = DALI_RECEIVING_DATA;
                //TIM4->CR1 &= ~TIM4_CR1_CEN;
                bsp_ext9_5_config();//rx exit interrpt config//enable EXTI
            }
        }
    }

    s_dali_controler.tick_count++;  
#endif


#ifdef DALI_SLAVE	
    //access to the routine just every 4 ticks = every half bit
    if ((s_dali_controler.tick_count & 0x03) == 0) {
        if (s_dali_controler.tick_count < 104) {
            // settling time between forward and backward frame
            if (s_dali_controler.tick_count < 32) {
                s_dali_controler.tick_count++;
                return;
            }

            // start of the start bit
            // 32 ticks = 8*Te time = delay between forward and backward message frame (1*Te time must be added as half of stop bit)
            if (s_dali_controler.tick_count == 32) {
                _set_dali_out(FALSE);
                s_dali_controler.tick_count++;
                return;
            }

            // edge of the start bit
            if (s_dali_controler.tick_count == 36) {
                _set_dali_out(TRUE);
                s_dali_controler.tick_count++;
                return;
            }

            // bit value (edge) selection
            bit_value = ((answer >> (7-s_dali_controler.bit_count)) & 0x01);

            // Every half bit -> Manchester coding
            if (!((s_dali_controler.tick_count - 32) & 0x0007)) { // div by 8
                /* former value of bit = new value of bit */
                if (_get_dali_out() == bit_value) {
                    _set_dali_out((1 - bit_value));
                }
            }

            // Generate edge for actual bit
            if (!((s_dali_controler.tick_count - 36) & 0x0007)) {
                _set_dali_out(bit_value);
                s_dali_controler.bit_count++;
            }
        } else { // end of data byte, start of stop bits
            if (s_dali_controler.tick_count == 104) {
                _set_dali_out(TRUE); // start of stop bit
            }

            // end of stop bits, no settling time
            if (s_dali_controler.tick_count == 120) {
                s_dali_controler.dali_flag = DALI_RECEIVING_DATA;
                //TIM4->CR1 &= ~TIM4_CR1_CEN;
                bsp_ext9_5_config();//rx exit interrpt config//enable EXTI      
            }
        }
    }
    s_dali_controler.tick_count++;
#endif
}

void dali_receive_data(void) 
{
    //SYS_DEBUG("x");

    // null variables
    if (flag_exi == 0) {
        s_dali_controler.address    = 0;
        s_dali_controler.dataByte   = 0;
        s_dali_controler.bit_count  = 0;
        s_dali_controler.tick_count = 0;
        s_dali_controler.former_val = TRUE;
        flag_exi = 1;

        // setup flag
        s_dali_controler.dali_flag = DALI_RECEIVING_DATA;
        // disable external interrupt on DALI in port
        //DrvGPIO_DisableInt(DALI_RX_PORT,DALI_RX_PIN);
    }
}

void dali_receive_tick(void) 
{
#ifdef DALI_HOST
    // Because of the structure of current amplifier, input has
    // to be negated
    static INT8U actual_val;
    actual_val = _get_dali_in();
    s_dali_controler.tick_count++;

    // edge detected
    if (actual_val != s_dali_controler.former_val) {
        switch (s_dali_controler.bit_count) {
            case 0:
                if (s_dali_controler.tick_count > 2) {
                    s_dali_controler.tick_count = 0;
                    s_dali_controler.bit_count  = 1; // start bit
                }
                break;
            case 9:      // 1st stop bit
                if (s_dali_controler.tick_count > 6) {// stop bit error, no edge should exist
                    s_dali_controler.dali_flag = DALI_ERR;
                }
                break;
            case 10:      // 2nd stop bit
                s_dali_controler.dali_flag = DALI_ERR; // stop bit error, no edge should exist
                break;
            default:      // other bits
                if (s_dali_controler.tick_count > 6) {
                    // store bit in data byte
                    s_dali_controler.dataByte |= (actual_val << (8-s_dali_controler.bit_count));
                    s_dali_controler.bit_count++;
                    s_dali_controler.tick_count = 0;
                }
                break;
        }
    } else { // voltage level stable
        switch (s_dali_controler.bit_count) {
            case 0:
                if (s_dali_controler.tick_count == 8) { // too long start bit
                    s_dali_controler.dali_flag = DALI_ERR;
                }
                break;
            case 9:
                // First stop bit
                if (s_dali_controler.tick_count == 8) {
                    if (actual_val==0) { // wrong level of stop bit
                        s_dali_controler.dali_flag = DALI_ERR;
                        flag_exi = 0;
                    } else {
                        s_dali_controler.bit_count++;
                        s_dali_controler.tick_count = 0;
                    }
                }
                break;
            case 10:
                // Second stop bit
                if (s_dali_controler.tick_count == 10) {
                    s_dali_controler.dali_flag = DALI_NO_ACTION;
                    //bsp_ext9_5_config();//rx exit interrpt config//enable EXTI
                    flag_exi=0;
                    //SYS_DEBUG("dali receive data:%x\r\n",s_dali_controler.dataByte);
                    //TIM4->CR1 &= ~TIM4_CR1_CEN;
                    //DataReceivedCallback(glbRtuPara->dali_controler.address,glbRtuPara->dali_controler.dataByte);
                }
                break;
            default: // normal bits
                if (s_dali_controler.tick_count==10) { // too long delay before edge
                    s_dali_controler.dali_flag = DALI_ERR;
                    flag_exi=0;
                }
                break;
        }
    }
    s_dali_controler.former_val = actual_val;

    if (s_dali_controler.dali_flag == DALI_ERR) {
        s_dali_controler.dali_flag = DALI_NO_ACTION;
        //bsp_ext9_5_config();//rx exit interrpt config//enable EXTI
        flag_exi = 0;
        //TIM4->CR1 &= ~TIM4_CR1_CEN;
    }
#endif

#ifdef DALI_SLAVE
    // Because of the structure of current amplifier, input has
    // to be negated
    static INT8U actual_val;

    actual_val = _get_dali_in();
    s_dali_controler.tick_count++;

    // edge detected
    if (actual_val != s_dali_controler.former_val) {
        switch (s_dali_controler.bit_count) {
            case 0:
                if (s_dali_controler.tick_count > 2) {
                    s_dali_controler.tick_count = 0;
                    s_dali_controler.bit_count  = 1; // start bit
                }
                break;
            case 17:      // 1st stop bit
                if(s_dali_controler.tick_count > 6) { // stop bit error, no edge should exist
                    s_dali_controler.dali_flag = DALI_ERR;
                }
                break;
            case 18:      // 2nd stop bit
                s_dali_controler.dali_flag = DALI_ERR; // stop bit error, no edge should exist
                break;
            default:      // other bits
                if (s_dali_controler.tick_count > 6) {
                    if (s_dali_controler.bit_count < 9) {// store bit in address byte
                        s_dali_controler.address |= (actual_val << (8-s_dali_controler.bit_count));
                    } else {            // store bit in data byte
                        s_dali_controler.dataByte |= (actual_val << (16-s_dali_controler.bit_count));
                    }
                    s_dali_controler.bit_count++;
                    s_dali_controler.tick_count = 0;
                }
                break;
        }
    } else { // voltage level stable
        
        switch (s_dali_controler.bit_count) {
            case 0:
                if (s_dali_controler.tick_count == 8) { // too long start bit
                    s_dali_controler.dali_flag = DALI_ERR;
                    SYS_DEBUG("e0\r\n");
                }
                break;
            case 17:
                // First stop bit
                if (s_dali_controler.tick_count == 8) {
                    if (actual_val == 0) {// wrong level of stop bit
                        s_dali_controler.dali_flag = DALI_ERR;
                        SYS_DEBUG("e17\r\n");
                    } else {
                        s_dali_controler.bit_count++;
                        s_dali_controler.tick_count = 0;
                    }
                }
                break;
            case 18:
                // Second stop bit
                if (s_dali_controler.tick_count == 10) {
                    s_dali_controler.dali_flag = DALI_NO_ACTION;
                    SYS_DEBUG("dali receive addr:%x,data:%x\r\n",s_dali_controler.address,s_dali_controler.dataByte);
                    flag_exi = 0; 
                    //DrvGPIO_EnableInt(DALI_RX_PORT,DALI_RX_PIN, E_IO_FALLING,E_MODE_EDGE);//enable EXTI
                    //TIM4->CR1 &= ~TIM4_CR1_CEN;
                    //DataReceivedCallback(glbRtuPara->dali_controler.address,glbRtuPara->dali_controler.dataByte);
                }
                break;
            default: // normal bits
                if (s_dali_controler.tick_count == 10) { // too long delay before edge
                    s_dali_controler.dali_flag = DALI_ERR;
                    SYS_DEBUG("dali receive err1 addr:%x,data:%x\r\n",s_dali_controler.address,s_dali_controler.dataByte);
                    flag_exi = 0; 	
                }
                break;
        }
    }
    
    s_dali_controler.former_val = actual_val;
    
    if (s_dali_controler.dali_flag == DALI_ERR) {
        s_dali_controler.dali_flag = DALI_NO_ACTION;
        //DrvGPIO_EnableInt(DALI_RX_PORT,DALI_RX_PIN, E_IO_FALLING,E_MODE_EDGE);//enable EXTI
        SYS_DEBUG("dali receive err2 addr:%x,data:%x\r\n",s_dali_controler.address,s_dali_controler.dataByte);
        flag_exi = 0; 
        //TIM4->CR1 &= ~TIM4_CR1_CEN;
    }
#endif
}

/*******************************************************************
** 函数名:      dali_auto_address
** 函数描述:    主机给从机分配地址
** 参数:        none
** 返回:        true or false
********************************************************************/
INT8U dali_auto_address(void)
{
	static INT8U addressingStep = 0, addrCnt = 0;//, flag_addrIni = 0;
	static INT8U addrIniVal = 0, retryNum = 0, flagSuccess = 0;
	static INT32U addrCmp = 0x800000, addrHigher = 0xFFFFFF, addrLower = 0;
    
	if (flagSuccess) {
        return TRUE;
    }
    
    if (retryNum > 5) {/* 超过次数 */
        return ERR;
    }
    
	switch (addressingStep) {
		case 0:
			SYS_DEBUG("#=%d\n", addressingStep);
			dali_send_data(DALI_ADDR_INI|addrIniVal, TRUE);
			dali_send_data(DALI_ADDR_INI|addrIniVal, TRUE);
			addressingStep++;
			break;
		case 1:
			SYS_DEBUG("#=%d\n", addressingStep);
			dali_send_data(DALI_ADDR_RAND, TRUE);
			dali_send_data(DALI_ADDR_RAND, TRUE);
			addressingStep++;
			break;			
		case 2:
			SYS_DEBUG("#=%d\n", addressingStep);
			addrCmp = 0xFFFFFF;
			_cmp_address(addrCmp);
			if (s_dali_controler.dataByte == 0) {
				dali_send_data(DALI_ADDR_EXIT, FALSE);
				SYS_DEBUG("addressing is done\n");
				flagSuccess = 1;
				return TRUE;
			} else if (s_dali_controler.dataByte == 0xff) {
				addrCmp    = 0x800000;
				addrHigher = 0xFFFFFF;
				addrLower  = 0x0;
				addressingStep++;
			}
			break;
		case 3:
			SYS_DEBUG("#=%d\n", addressingStep);
			if ((addrHigher == addrCmp) || (addrLower == addrCmp)) {
				_cmp_address(addrCmp);
				if (s_dali_controler.dataByte == 0xff) {//find rand addr
					addrCnt++;					
					//dali_send_data(DALI_CMP_PHY_SEL,0,2,TRUE);	
					dali_send_data(DALI_SHORT_ADDR_SET|(addrCnt*2+1), TRUE);						
					dali_send_data(DALI_CHK_SHORT_ADDR|(addrCnt*2+1), TRUE);	
					if (s_dali_controler.dataByte == 0x00) {
						retryNum++;
						addrCnt--;
						SYS_DEBUG("set short addr=%0.2x err retryNum=%d\n",addrCnt,retryNum);
					} else if (s_dali_controler.dataByte == 0xff) {
						SYS_DEBUG("set short addr=%0.2x ok\n",addrCnt);
						dali_send_data(DALI_ADDR_EXIT, TRUE);
						addrIniVal = 0xff;//change to increse addressing
						addressingStep = 0;
					}
				} else {
					addrLower = addrCmp;
					addrCmp  += (addrHigher-addrLower+1)/2;	
				}
			} else {
				_cmp_address(addrCmp);
				if (s_dali_controler.dataByte == 0) {
					addrLower = addrCmp;
					addrCmp  += (addrHigher-addrLower+1)/2;					
				} else if (s_dali_controler.dataByte == 0xff) {
					addrHigher = addrCmp;
					addrCmp   -= (addrHigher-addrLower+1)/2;					
				}
			}
			break;
		//defalut:
		//	break;
	}
	
	return FALSE;
}

/*

//包装协议
ker_enum_type dev_dali_package(box_type * box,package_type* pack,glbRtuPara_type * glbRtuPara)
{
	#ifdef USE_DALI
	#endif
	return OK;
	
}
//发送
ker_enum_type dev_dali_send(box_type * box)
{
	#ifdef USE_DALI
	 dali_send_data(box->out_buf);	 
	 //dali_state = DALI_SEND_START;
	#endif
	return OK;
	
}

//解析协议
ker_enum_type dev_dali_recv(box_type * box,package_type* pack,glbRtuPara_type * glbRtuPara)
{
	#ifdef USE_DALI
	#endif
	return ERR;
}

//取出协议
ker_enum_type dev_dali_unpackage(package_type* pack,glbRtuPara_type * glbRtuPara)
{
	#ifdef USE_DALI
	#endif
	return OK;
}

//模块自己的命令
ker_enum_type dev_dali_cmd(box_type * box,package_type* pack,glbRtuPara_type * glbRtuPara)
{
	#ifdef USE_DALI
	#endif
	return OK;
}

void dali_cmd_debug(INT8U test)
{
       #ifdef USE_DALI
	SYS_DEBUG("dali_cmd_debug\r\n");
	data_box.out_buf[0]=0xff;
	switch(test)
	{
		case '0':
			data_box.out_buf[1]=0x0;
			dali_send_data(data_box.out_buf);
			break;
		case '1':
			data_box.out_buf[1]=0x1;
			dali_send_data(data_box.out_buf);			
			break;
		case '2':
			data_box.out_buf[1]=0x2;
			dali_send_data(data_box.out_buf);			
			break;
		case '3':
			data_box.out_buf[1]=0x3;
			dali_send_data(data_box.out_buf);			
			break;
		case '4':
			data_box.out_buf[1]=0x4;
			dali_send_data(data_box.out_buf);			
			break;
		case '5':
			data_box.out_buf[1]=0x5;
			dali_send_data(data_box.out_buf);			
			break;
		case '6':
			data_box.out_buf[1]=0x6;
			dali_send_data(data_box.out_buf);			
			break;
		case '7':
			data_box.out_buf[1]=0x7;
			dali_send_data(data_box.out_buf);			
			break;
		case '8':
			data_box.out_buf[1]=0x8;
			dali_send_data(data_box.out_buf);			
			break;
		case '9':
			data_box.out_buf[1]=0x9;
			dali_send_data(data_box.out_buf);			
			break;
		default:
			break;
	}
	#endif
}
*/

#endif /* end of EN_DALI */

