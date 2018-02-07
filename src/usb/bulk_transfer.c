#include "bulk_transfer.h"
#include "diskio.h"
#include "usb_host.h"
#include "MDR32Fx.h"                    // Device header
#include "MDR32F9Qx_usb.h"              // Milandr::Drivers:USB
#include "ctrl_transfer.h"

const setup_pck BULK_ONLY_MASS_STORAGE_RESET = {
	TSF_DIR_HST_TO_DEV | REQ_TYPE_CLASS | RECIPIENT_ITF,		/* bmRequestType */
	0xFF,																										/* bRequest */
	0x0000,																									/* wValue */
	0x0000,																									/* wIndex */
	0x0000																									/* wLength */
};


// Рабочие переменные
static uint8_t 						*cbw_ptr; 
static uint8_t 						*bt_data_ptr; 
static uint8_t 						*csw_ptr; 
static uint32_t 					bt_stages_num; 
static uint32_t 					bt_stage_bytes_num; 

// Резервные переменные	 
static uint8_t 						*cbw_ptr_res; 
static uint8_t 						*bt_data_ptr_res; 
static uint8_t 						*csw_ptr_res; 
static uint32_t 					bt_stages_num_res; 

blk_tsf_type_t 						bt_type;
blk_tsf_stage_t 					bt_stage;



uint8_t blk_tsf_buf[BULK_TRANSFER_BUFFER_LENGTH];
DWORD LBA;
short transferlenth;
blk_only_csw_pck CSW;
blk_only_cbw_pck CBW;
CAPACITY10 RC10;


// Установка начальных значений передачи
void blk_tsf_setup(
	uint32_t 	_dev_addr, 				// Адрес устройства 
	uint8_t 	*_cbw_ptr, 				// Указатель на CBW 
	uint8_t  	*_bt_ptr, 				// Указатель на данные 
	uint8_t 	*_csw_ptr, 				// Указатель на CSW
	uint32_t 	_bt_stages_num, 		// Число стадий данных 
	uint32_t 	_bt_stage_bytes_num, 	// Количество байт в одной стадии данных 
	blk_tsf_type_t _bt_type) 			// Тип передачи (IN/OUT)
{
	dev_addr 						= _dev_addr;
	cbw_ptr_res					= _cbw_ptr;
	bt_data_ptr_res			= _bt_ptr;
	csw_ptr_res					= _csw_ptr;
	bt_stages_num_res		= _bt_stages_num;
	bt_stage_bytes_num	= _bt_stage_bytes_num;
	bt_type							= _bt_type;
	bt_stage						=  BLK_TSF_STAGE_CBW_BEGIN;
}
	 
blk_tsf_stage_t blk_tsf(void) 
{
	if(dev_conn_st == DEV_CONN_ST_DETACHED)
	{
		host_st = HOST_ST_WAIT_CONN;
		bt_stage = BLK_TSF_ERROR;
	}
	else 
	{
		switch(bt_stage) 
		{
			case BLK_TSF_STAGE_CBW_BEGIN: 
				cbw_ptr 	  	= cbw_ptr_res;
				bt_data_ptr 	= bt_data_ptr_res;
				csw_ptr 	  	= csw_ptr_res;
				bt_stages_num = bt_stages_num_res;
			
				pck_tsf_setup(dev_addr, ep_out, token);
				for (uint8_t i = 0; i < 31; i++) { 
					MDR_USB->HTXFD = *cbw_ptr++;
				}
				ans = TRANS_ANS_NO;
				bt_stage = BLK_TSF_STAGE_CBW_END;
				USB_SetHTXC(USB_HTXC_TREQ_Set);
				//delay_ms(10);
				break; 
			
			case BLK_TSF_STAGE_CBW_END: 
				if(ans != TRANS_ANS_NO) 
				{
					switch(ans)
					{
						case TRANS_ANS_ACK: 
							token = (token == TOKEN_TYPE_OUT_DATA0) ? TOKEN_TYPE_OUT_DATA1 : TOKEN_TYPE_OUT_DATA0; 
							if(bt_stages_num == 0) 
							{
								bt_stage = BLK_TSF_STAGE_CSW_BEGIN;
							}
							else 
							{
								bt_stage = BLK_TSF_STAGE_DATA_BEGIN;
							}
							break;
						case TRANS_ANS_STALL: 
							reset_recovery();
							bt_stage = BLK_TSF_STAGE_CBW_BEGIN;
							break;
						default:
							bt_stage = BLK_TSF_STAGE_CBW_BEGIN;
							break;
					}
				}
				break; 
					
			case BLK_TSF_STAGE_DATA_BEGIN: 
				if(bt_type == BLK_TSF_TYPE_IN) 
				{
					pck_tsf_setup(dev_addr, ep_in, TOKEN_TYPE_IN);
				}
				else 
				{
					if(ans != TRANS_ANS_ACK) 
					{
						bt_data_ptr -= bt_stage_bytes_num;
					}
					pck_tsf_setup(dev_addr, ep_out, token);
					for (uint32_t i = 0; i < bt_stage_bytes_num; i++) 
					{ 
						MDR_USB->HTXFD  = *bt_data_ptr++;
					}
				}
				ans = TRANS_ANS_NO;
				bt_stage = BLK_TSF_STAGE_DATA_END;
				USB_SetHTXC(USB_HTXC_TREQ_Set); 	
				//delay_ms(10);
				break; 
			
			case BLK_TSF_STAGE_DATA_END:
				
				
				if(ans != TRANS_ANS_NO) 
				{
					switch(ans)
					{
						case TRANS_ANS_ACK:
							if(bt_type == BLK_TSF_TYPE_OUT)
							{
								token = (token == TOKEN_TYPE_OUT_DATA0) ? TOKEN_TYPE_OUT_DATA1 : TOKEN_TYPE_OUT_DATA0;
								if(bt_stages_num > 1)
								{
									bt_stage = BLK_TSF_STAGE_DATA_BEGIN;
									bt_stages_num--;
								}
								else
									bt_stage = BLK_TSF_STAGE_CSW_BEGIN;
							}
							else
							{
								bt_stage = BLK_TSF_STAGE_CBW_BEGIN;
							}
							break;
						case TRANS_ANS_DATA0:
						case TRANS_ANS_DATA1:
							if(bt_type == BLK_TSF_TYPE_IN)
							{
									for (uint32_t i = 0; i < bt_stage_bytes_num; i++)
									{								
										*bt_data_ptr++ = MDR_USB->HRXFD;
									}
									if(bt_stages_num > 1)
									{
										bt_stage = BLK_TSF_STAGE_DATA_BEGIN;
										bt_stages_num--;
									}
									else
									{
										bt_stage = BLK_TSF_STAGE_CSW_BEGIN;
									}
							}
							else
							{
								bt_stage = BLK_TSF_STAGE_CBW_BEGIN;
							}
							break;																								
						case TRANS_ANS_STALL:
							reset_recovery();
							bt_stage = BLK_TSF_STAGE_CBW_BEGIN;
							break;
						case TRANS_ANS_NAK:
							bt_stage = BLK_TSF_STAGE_DATA_BEGIN;
							break;
						default:
							bt_stage = BLK_TSF_STAGE_DATA_BEGIN;
							break;
					}
				}
				break; 
				
			case BLK_TSF_STAGE_CSW_BEGIN:
				pck_tsf_setup(dev_addr, ep_in, TOKEN_TYPE_IN);
				bt_stage = BLK_TSF_STAGE_CSW_END;
				ans = TRANS_ANS_NO;
				USB_SetHTXC(USB_HTXC_TREQ_Set); 
				//delay_ms(10);
				break; 
				
			case BLK_TSF_STAGE_CSW_END:
				if(ans != TRANS_ANS_NO)
				{
					switch(ans)
					{				
						case TRANS_ANS_DATA0:
						case TRANS_ANS_DATA1:
							for(uint8_t i = 0; i < 13; i++)
							{							
								*csw_ptr++ = MDR_USB->HRXFD;
							}
							bt_stage = BLK_TSF_COMPLETE;
							break;
						case TRANS_ANS_STALL:
							reset_recovery();
							bt_stage = BLK_TSF_STAGE_CBW_BEGIN;
							break;
						case TRANS_ANS_NAK:
							bt_stage = BLK_TSF_STAGE_CSW_BEGIN;
							break;						
						default:
							bt_stage = BLK_TSF_STAGE_CSW_BEGIN;
							break;
					}
				}
				break; 
		}
	}
	return bt_stage;
}


/*Bulk only function to write 1 sector on LBA*/
blk_tsf_stage_t BOT_out(void) {	

		CBW.dSignature 			= BOT_CBW_SIGNATURE;
		CBW.dTag 						= 0x140088;
		CBW.dDataLength 		= 0x200;
		CBW.bmFlags 				= 0;
		CBW.bLUN 						= 0;
		CBW.bCBLength 			= 0xA;
		CBW.CB[0] 					= SCSI_WRITE10;/*Command*/
		CBW.CB[1] 					= 0x00;/*reserved*/
		CBW.CB[2] 					= ((uint8_t *)&LBA)[3];/*LBA*/
		CBW.CB[3] 					= ((uint8_t *)&LBA)[2];
		CBW.CB[4] 					= ((uint8_t *)&LBA)[1];
		CBW.CB[5] 					= ((uint8_t *)&LBA)[0];
		CBW.CB[6] 					= 0x00;/*reserved*/
		CBW.CB[7] 					= ((uint8_t *)&transferlenth)[1];/*lenth*/
		CBW.CB[8] 					= ((uint8_t *)&transferlenth)[0];
		CBW.CB[9] 					= 0x00;/*reserved*/
		CBW.CB[10] 					= 0x00;
		CBW.CB[11] 					= 0x00;
		CBW.CB[12] 					= 0x00;
		CBW.CB[13] 					= 0x00;
		CBW.CB[14] 					= 0x00;
		CBW.CB[15] 					= 0x00;
		
		CSW.dSignature			= 0;
		CSW.dTag 						= 0;
		CSW.dDataResidue 		= 0;
		CSW.bStatus 				= 0;

		blk_tsf_setup(MASS_STORAGE_DEVICE_ADDRESS, (uint8_t *)&CBW, blk_tsf_buf, (uint8_t *)&CSW, 64, 8, BLK_TSF_TYPE_OUT);
		while((blk_tsf() != BLK_TSF_COMPLETE) && (blk_tsf() != BLK_TSF_ERROR));
		
		return bt_stage;
}


/*Bulk only function to read 1 sector on LBA*/
blk_tsf_stage_t BOT_in(void) {
		CBW.dSignature 			= BOT_CBW_SIGNATURE;
		CBW.dTag 						= 0x140088;
		CBW.dDataLength 		= 0x200;
		CBW.bmFlags 				= 0x80;
		CBW.bLUN 						= 0;
		CBW.bCBLength 			= 0xA;
		CBW.CB[0] 					= SCSI_READ10;/*Command*/
		CBW.CB[1] 					= 0x00;/*reserved*/
		CBW.CB[2] 					= ((uint8_t *)&LBA)[3];/*LBA*/
		CBW.CB[3] 					= ((uint8_t *)&LBA)[2];
		CBW.CB[4] 					= ((uint8_t *)&LBA)[1];
		CBW.CB[5] 					= ((uint8_t *)&LBA)[0];
		CBW.CB[6] 					= 0x00;/*reserved*/
		CBW.CB[7] 					= ((uint8_t *)&transferlenth)[1];/*lenth*/
		CBW.CB[8] 					= ((uint8_t *)&transferlenth)[0];
		CBW.CB[9] 					= 0x00;/*reserved*/
		CBW.CB[10] 					= 0x00;
		CBW.CB[11] 					= 0x00;
		CBW.CB[12] 					= 0x00;
		CBW.CB[13] 					= 0x00;
		CBW.CB[14] 					= 0x00;
		CBW.CB[15] 					= 0x00;
		
		CSW.dSignature 			= 0;
		CSW.dTag 						= 0;
		CSW.dDataResidue 		= 0;
		CSW.bStatus 				= 0;
	
		blk_tsf_setup(MASS_STORAGE_DEVICE_ADDRESS, (uint8_t *)&CBW, blk_tsf_buf, (uint8_t *)&CSW, 8, 64, BLK_TSF_TYPE_IN);
		while((blk_tsf() != BLK_TSF_COMPLETE) && (blk_tsf() != BLK_TSF_ERROR));
		
		return bt_stage;
}		


/*Bulk only function initialize device by reading capacity*/
blk_tsf_stage_t BOT_init(void) {
		CBW.dSignature 			= BOT_CBW_SIGNATURE;
		CBW.dTag 						= 0x140088;
		CBW.dDataLength 		= 0x08;
		CBW.bmFlags 				= 0x80;
		CBW.bLUN 						= 0;
		CBW.bCBLength 			= 0xA;
		CBW.CB[0] 					= SCSI_READ_CAPACITY10;/*Command*/
		CBW.CB[1]	 					= 0x00;/*reserved*/
		CBW.CB[2] 					= 0x00;/*LBA*/
		CBW.CB[3] 					= 0x00;
		CBW.CB[4] 					= 0x00;
		CBW.CB[5] 					= 0x00;
		CBW.CB[6] 					= 0x00;/*reserved*/
		CBW.CB[7] 					= 0x00;
		CBW.CB[8] 					= 0x00;/*lenth*/
		CBW.CB[9] 					= 0x00;/*reserved*/
		CBW.CB[10] 					= 0x00;
		CBW.CB[11] 					= 0x00;
		CBW.CB[12] 					= 0x00;
		CBW.CB[13] 					= 0x00;
		CBW.CB[14] 					= 0x00;
		CBW.CB[15] 					= 0x00;
		
		CSW.dSignature 			= 0;
		CSW.dTag 						= 0;
		CSW.dDataResidue 		= 0;
		CSW.bStatus 				= 0;
		
		blk_tsf_setup(MASS_STORAGE_DEVICE_ADDRESS, (uint8_t *)&CBW, blk_tsf_buf, (uint8_t *)&CSW, 1, 8, BLK_TSF_TYPE_IN);
		while((blk_tsf() != BLK_TSF_COMPLETE) && (blk_tsf() != BLK_TSF_ERROR));
		
		return bt_stage;
}


/*Bulk only function returning status of the device*/
blk_tsf_stage_t BOT_stat(void) {
		CBW.dSignature 			= BOT_CBW_SIGNATURE;
		CBW.dTag 						= 0x140088;
		CBW.dDataLength 		= 0x00;
		CBW.bmFlags 				= 0x00;
		CBW.bLUN 						= 0;
		CBW.bCBLength 			= 0x6;
	
		CBW.CB[0] 					= SCSI_TEST_UNIT_READY;/*Command*/
		CBW.CB[1] 					= 0x00;/*reserved*/
		CBW.CB[2] 					= 0x00;/*LBA*/
		CBW.CB[3] 					= 0x00;
		CBW.CB[4] 					= 0x00;
		CBW.CB[5] 					= 0x00;
		CBW.CB[6] 					= 0x00;/*reserved*/
		CBW.CB[7] 					= 0x00;
		CBW.CB[8] 					= 0x00;/*lenth*/
		CBW.CB[9] 					= 0x00;/*reserved*/
		CBW.CB[10] 					= 0x00;
		CBW.CB[11] 					= 0x00;
		CBW.CB[12] 					= 0x00;
		CBW.CB[13] 					= 0x00;
		CBW.CB[14] 					= 0x00;
		CBW.CB[15] 					= 0x00;
	
		CSW.dSignature 			= 0;
		CSW.dTag 						= 0;
		CSW.dDataResidue 		= 0;
		CSW.bStatus 				= 0;
		
		blk_tsf_setup(MASS_STORAGE_DEVICE_ADDRESS, (uint8_t *)&CBW, blk_tsf_buf, (uint8_t *)&CSW, 0, 0, BLK_TSF_TYPE_IN);
		while((blk_tsf() != BLK_TSF_COMPLETE) && (blk_tsf() != BLK_TSF_ERROR));		

		return bt_stage;
}


blk_tsf_stage_t BOT_request_sense(void) {
		CBW.dSignature 			= BOT_CBW_SIGNATURE;
		CBW.dTag 						= 0x140088;
		CBW.dDataLength 		= 0x12;
		CBW.bmFlags 				= 0x80;
		CBW.bLUN 						= 0;
		CBW.bCBLength 			= 0xA;
		
		CBW.CB[0] 					= SCSI_REQUEST_SENSE;/*Command*/
		CBW.CB[1]						= 0x00;/*reserved*/
		CBW.CB[2] 					= 0x00;/*LBA*/
		CBW.CB[3] 					= 0x00;
		CBW.CB[4] 					= 0x12;
		CBW.CB[5] 					= 0x00;
		CBW.CB[6] 					= 0x00;/*reserved*/
		CBW.CB[7] 					= 0x00;
		CBW.CB[8] 					= 0x00;/*lenth*/
		CBW.CB[9]	 					= 0x00;/*reserved*/
		CBW.CB[10] 					= 0x00;
		CBW.CB[11] 					= 0x00;
		CBW.CB[12] 					= 0x00;
		CBW.CB[13] 					= 0x00;
		CBW.CB[14] 					= 0x00;
		CBW.CB[15] 					= 0x00;
		
		CSW.dSignature 			= 0;
		CSW.dTag 						= 0;
		CSW.dDataResidue 		= 0;
		CSW.bStatus 				= 0;
	
		blk_tsf_setup(MASS_STORAGE_DEVICE_ADDRESS, (uint8_t *)&CBW, blk_tsf_buf, (uint8_t *)&CSW, 1, 64, BLK_TSF_TYPE_IN);
		while((blk_tsf() != BLK_TSF_COMPLETE) && (blk_tsf() != BLK_TSF_ERROR));
		
		return bt_stage;
}

blk_tsf_stage_t BOT_inquiry(void) {
		CBW.dSignature 			= BOT_CBW_SIGNATURE;
		CBW.dTag 						= 0x140088;
		CBW.dDataLength 		= 0x24;
		CBW.bmFlags 				= 0x80;
		CBW.bLUN 						= 0;
		CBW.bCBLength 			= 0x6;
		
		CBW.CB[0] 					= SCSI_INQUIRY ;/*Command*/
		CBW.CB[1] 					= 0x00;/*reserved*/
		CBW.CB[2] 					= 0x00;/*LBA*/
		CBW.CB[3] 					= 0x00;
		CBW.CB[4] 					= 0x24;
		CBW.CB[5] 					= 0x00;
		CBW.CB[6] 					= 0x00;/*reserved*/
		CBW.CB[7] 					= 0x00;
		CBW.CB[8] 					= 0x00;/*lenth*/
		CBW.CB[9] 					= 0x00;/*reserved*/
		CBW.CB[10] 					= 0x00;
		CBW.CB[11] 					= 0x00;
		CBW.CB[12] 					= 0x00;
		CBW.CB[13] 					= 0x00;
		CBW.CB[14] 					= 0x00;
		CBW.CB[15] 					= 0x00;
		
		CSW.dSignature 			= 0;
		CSW.dTag 						= 0;
		CSW.dDataResidue 		= 0;
		CSW.bStatus 				= 0;
	
		blk_tsf_setup(MASS_STORAGE_DEVICE_ADDRESS, (uint8_t *)&CBW, blk_tsf_buf, (uint8_t *)&CSW, 1, 64, BLK_TSF_TYPE_IN);
		while((blk_tsf() != BLK_TSF_COMPLETE) && (blk_tsf() != BLK_TSF_ERROR));
		
		return bt_stage;
}
