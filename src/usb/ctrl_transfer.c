#include "ctrl_transfer.h"
#include "usb_host.h"
#include "MDR32Fx.h"                    // Device header
#include "MDR32F9Qx_usb.h"              // Milandr::Drivers:USB
#include <stddef.h>


/* Рабочие переменные	*/
static uint8_t  									 *req_ptr; 						/* Указатель на запрос */ 
static uint8_t  									 *ct_data_ptr; 				/* Указатель на данные */
static uint32_t 									 ct_stage_bytes_num;	/* Количество байт в одной стадии данных */
static uint32_t 									 ct_stages_num; 			/* Количество стадий данных */


/* Смысл резервных переменных: в случае ошибок в процессе передачи нужно возвращаться на предыдущие стадии, 
	 а рабочие переменные уже указывают не туда, и нужно их переинициализировать из резервных переменных. */
	 
/* Резервные переменные */	 
static uint8_t  									 *req_ptr_res; 
static uint8_t  									 *ct_data_ptr_res; 
static uint32_t 									 ct_stages_num_res; 

ctrl_tsf_type_t 					 				 ct_type;
ctrl_tsf_stage_t 					 				 ct_st;



const setup_pck SETUP_PCK_SET_ADDR = {
	TSF_DIR_HST_TO_DEV | REQ_TYPE_STANDARD | RECIPIENT_DEV,		/* bmRequestType */
	STD_REQ_SET_ADDR, 																				/* bRequest */
	MASS_STORAGE_DEVICE_ADDRESS,															/* wValue */
	0x0000,																										/* wIndex */
	0x0000																										/* wLength */
};

const setup_pck SETUP_PCK_GET_DESC = {
	TSF_DIR_DEV_TO_HST | REQ_TYPE_STANDARD | RECIPIENT_DEV,		/* bmRequestType */
	STD_REQ_GET_DESC,																					/* bRequest */
 (STD_DESC_DEV << 8),																		   	/* wValue */
	0x0000,																										/* wIndex */
	0x0012																										/* wLength */
};			

const setup_pck SETUP_PCK_GET_DESC_CFG = {
	TSF_DIR_DEV_TO_HST | REQ_TYPE_STANDARD | RECIPIENT_DEV,
	STD_REQ_GET_DESC,
 (STD_DESC_CFG << 8),
	0x0000,
	0x00FF
};



setup_pck SETUP_PCK_SET_CFG = {
	TSF_DIR_HST_TO_DEV | REQ_TYPE_STANDARD | RECIPIENT_DEV,
	STD_REQ_SET_CFG,
	0x0000,
	0x0000,
	0x0000
};

setup_pck SETUP_PCK_SET_ITF = {
	TSF_DIR_HST_TO_DEV | REQ_TYPE_STANDARD | RECIPIENT_ITF,
	STD_REQ_SET_ITF,
	0x0000,
	0x0000,
	0x0000
};

setup_pck SETUP_PCK_BULK_REQ = {
	TSF_DIR_HST_TO_DEV | REQ_TYPE_STANDARD | RECIPIENT_EP, 
	STD_REQ_CLEAR_FEATURE, 
	0x0000,
	0x0000,
	0x0000
};

void ctrl_tsf_setup(
		uint8_t 							_dev_addr,							 
		uint8_t 							*_req_ptr,							 
		uint8_t 							*_ct_data_ptr,						 
		uint32_t							_ct_stage_bytes_num,			 
		uint32_t 							_ct_stages_num,						 
		ctrl_tsf_type_t 			_ct_type) 
{
	dev_addr 		    	 = _dev_addr;
	req_ptr_res     	 = _req_ptr;
	ct_data_ptr_res 	 = _ct_data_ptr;
	ct_stage_bytes_num = _ct_stage_bytes_num;
	ct_stages_num_res  = _ct_stages_num;
	ct_type						 = _ct_type;
	ct_st							 = CTRL_TSF_STAGE_SETUP_BEGIN;		 
}


ctrl_tsf_stage_t ctrl_tsf(void) {
	if(dev_conn_st == DEV_CONN_ST_DETACHED) {
		ct_st = CTRL_TSF_ERROR;
		host_st = HOST_ST_WAIT_CONN;
	}
	else {
		switch(ct_st) {
		case CTRL_TSF_STAGE_SETUP_BEGIN: {
				/*  */
				req_ptr = req_ptr_res;
				ct_data_ptr = ct_data_ptr_res;
				ct_stages_num = ct_stages_num_res;
				
				MDR_USB->HTXFC |= 1;
				
				/*  */
				pck_tsf_setup(dev_addr, 0, TOKEN_TYPE_SETUP);
			
				for (uint8_t i = 0; i < 8; i++) { 
					MDR_USB->HTXFD = *req_ptr++;
				}
				ans = TRANS_ANS_NO;
				ct_st = CTRL_TSF_STAGE_SETUP_END;
				USB_SetHTXC(USB_HTXC_TREQ_Set);					/* Начать транзакцию */
				//delay_ms(10);
			}
			break; 
			
			
		case CTRL_TSF_STAGE_SETUP_END: {
				if(ans != TRANS_ANS_NO) {
					switch(ans) {
					case TRANS_ANS_ACK: {
							if(ct_stages_num == 0) {
								ct_st = CTRL_TSF_STAGE_STATUS_BEGIN;
							}
							else { 
								ct_st = CTRL_TSF_STAGE_DATA_BEGIN;
							}
						}
						break;
					default: {
							ct_st = CTRL_TSF_STAGE_SETUP_BEGIN;
						}
						break;
					}	
				}
			}
			break; 
			
			
		case CTRL_TSF_STAGE_DATA_BEGIN: {	
				if(ct_type == CTRL_TSF_TYPE_READ) {
					pck_tsf_setup(dev_addr, 0, TOKEN_TYPE_IN);
				}
				else {
					for(uint32_t i = 0; i < ct_stage_bytes_num; i++) {
						MDR_USB->HTXFD = *ct_data_ptr++;
					}
					pck_tsf_setup(dev_addr, 0, token);
				}
				ans = TRANS_ANS_NO;
				ct_st = CTRL_TSF_STAGE_DATA_END;
				USB_SetHTXC(USB_HTXC_TREQ_Set);				/* Начать транзакцию */
				//delay_ms(10);
			}
			break;
			
				
		case CTRL_TSF_STAGE_DATA_END: {				
				if(ans != TRANS_ANS_NO) {
					switch(ans) {
					case TRANS_ANS_ACK: {
							if(ct_type == CTRL_TSF_TYPE_WRITE) {
								token = (token == TOKEN_TYPE_OUT_DATA0) ? TOKEN_TYPE_OUT_DATA1 : TOKEN_TYPE_OUT_DATA0;
								ct_st = CTRL_TSF_STAGE_STATUS_BEGIN;
							}
						}
						break;	
					case TRANS_ANS_DATA0:
					case TRANS_ANS_DATA1: {	
							if(ct_type == CTRL_TSF_TYPE_READ) {
								for (uint32_t i = 0; i < ct_stage_bytes_num; i++) { 
									*ct_data_ptr++ = MDR_USB->HRXFD;
								}
								if(ct_stages_num > 1) {
									ct_st = CTRL_TSF_STAGE_DATA_BEGIN;
									ct_stages_num--;
								}
								else {
									ct_st = CTRL_TSF_STAGE_STATUS_BEGIN;
								}
							}
							else {
								ct_st = CTRL_TSF_STAGE_SETUP_BEGIN;
							}
						}
						break;
					case TRANS_ANS_STALL: {
							ct_st = CTRL_TSF_STAGE_SETUP_BEGIN;
						}
						break;
					default: {
							ct_st = CTRL_TSF_STAGE_DATA_BEGIN;
						}
						break;
					}
				}
			}
			break;
					
		case CTRL_TSF_STAGE_STATUS_BEGIN: {
				if(ct_type == CTRL_TSF_TYPE_READ) {
					pck_tsf_setup(dev_addr, 0, token);
				}
				else {
					pck_tsf_setup(dev_addr, 0, TOKEN_TYPE_IN);
				}
				ct_st = CTRL_TSF_STAGE_STATUS_END;
				ans = TRANS_ANS_NO;
				USB_SetHTXC(USB_HTXC_TREQ_Set);				/* Начать транзакцию */				
			}
			break; 
					
				
		case CTRL_TSF_STAGE_STATUS_END: {
					if(ans != TRANS_ANS_NO) {			
						switch(ans) {
						case TRANS_ANS_ACK: {
								if(ct_type == CTRL_TSF_TYPE_READ) {
									ct_st = CTRL_TSF_COMPLETE;
									token = (token == TOKEN_TYPE_OUT_DATA0) ? TOKEN_TYPE_OUT_DATA1 : TOKEN_TYPE_OUT_DATA0;
									pck_tsf_setup(dev_addr, 0, token);
									USB_SetHTXC(USB_HTXC_TREQ_Set);				/* Начать транзакцию */
									token = (token == TOKEN_TYPE_OUT_DATA0) ? TOKEN_TYPE_OUT_DATA1 : TOKEN_TYPE_OUT_DATA0;
								}
							}
							break;
							
						case TRANS_ANS_DATA0:
						case TRANS_ANS_DATA1: {
							if(ct_type == CTRL_TSF_TYPE_WRITE) {
								ct_st = CTRL_TSF_COMPLETE;
							}
							break;
						case TRANS_ANS_STALL: {
								ct_st = CTRL_TSF_STAGE_SETUP_BEGIN;
							}
							break;
								
						default: {
								ct_st = CTRL_TSF_STAGE_STATUS_BEGIN;
							}
							break;
						}
					}
				}
			break;			
			}
		}
	}
	return ct_st;
}




/*Specific reset recovery*/
ctrl_tsf_stage_t reset_recovery(void) {	
		ctrl_tsf_setup(MASS_STORAGE_DEVICE_ADDRESS, (uint8_t *)&BULK_ONLY_MASS_STORAGE_RESET, NULL, 0, 0, CTRL_TSF_TYPE_WRITE);
		while((ctrl_tsf() != CTRL_TSF_COMPLETE) && (ctrl_tsf() != CTRL_TSF_ERROR));
		if(ct_st == CTRL_TSF_ERROR)
			return CTRL_TSF_ERROR;
		
		SETUP_PCK_BULK_REQ.wIndex = ep_in;
		ctrl_tsf_setup(MASS_STORAGE_DEVICE_ADDRESS, (uint8_t *)&SETUP_PCK_BULK_REQ, NULL, 0, 0, CTRL_TSF_TYPE_WRITE);
		while((ctrl_tsf() != CTRL_TSF_COMPLETE) && (ctrl_tsf() != CTRL_TSF_ERROR));
		if(ct_st == CTRL_TSF_ERROR)
			return CTRL_TSF_ERROR;
			
		SETUP_PCK_BULK_REQ.wIndex = ep_out;
		ctrl_tsf_setup(MASS_STORAGE_DEVICE_ADDRESS, (uint8_t *)&SETUP_PCK_BULK_REQ, NULL, 0, 0, CTRL_TSF_TYPE_WRITE);
		while((ctrl_tsf()!= CTRL_TSF_COMPLETE) && (ctrl_tsf() != CTRL_TSF_ERROR));
		if(ct_st == CTRL_TSF_ERROR)
			return CTRL_TSF_ERROR;
		
		return CTRL_TSF_COMPLETE;
}

