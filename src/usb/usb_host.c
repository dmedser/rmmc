#include "usb_host.h"
#include "MDR32Fx.h"                    // Device header
#include "MDR32F9Qx_usb.h"              // Milandr::Drivers:USB
#include "util.h"
#include "ctrl_transfer.h"
#include <stddef.h>
#include "ff.h"
#include "flash.h"

/* Статус подключаемого USB флеш-накопителя (UFD) */
dev_conn_st_t 	dev_conn_st; 
/* Состояние процесса инициализации подключенного UFD */
host_st_t		 		host_st;
/* Код ответа от UFD */
trans_ans_t			ans;
/* Тип токен-пакета */
token_type_t		token;
/* Буфер приема и передачи данных хоста */
uint8_t 				*data_ptr;
/* Адрес конечной точки приема подключенного UFD */
uint8_t 	      ep_in;
/* Адрес конечной точки передачи подключенного UFD */
uint8_t 	      ep_out;
/* Струкртура, описывающая дескриптор */
desc_t		      desc;
/* Адрес подключенного UFD */
uint8_t		      dev_addr;


/* Инициализация блока USB в режиме хост */
void usb_host_init(void) {
	/* Разрешение тактирования переферийного блока USB */
	MDR_RST_CLK->PER_CLOCK |= RST_CLK_PER_CLOCK_PCLK_EN_USB;
	
	/* Разрешение тактирования USB */
	MDR_RST_CLK->USB_CLOCK |= RST_CLK_USB_CLOCK_USB_CLK_EN;
	
	/* Задание источника тактового сигнала USB = PLLUSBo */
	MDR_RST_CLK->USB_CLOCK |= (RST_CLK_USB_CLOCK_USB_C2_SEL_PLL_USB << RST_CLK_USB_CLOCK_USB_C2_SEL_Pos) |
														(RST_CLK_USB_CLOCK_USB_C3_SEL_USB_C2 << RST_CLK_USB_CLOCK_USB_C3_SEL_Pos);
	
	
	/* Задание PLLUSBi = HSE (= 16 МГц на отладочной плате) */
	MDR_RST_CLK->USB_CLOCK |= (RST_CLK_USB_CLOCK_USB_C1_SEL_HSE << RST_CLK_USB_CLOCK_USB_C1_SEL_Pos);
	
	/* Задание коэффициента умножения PLLUSBMUL, PLLUSBo = PLLUSBi * (PLLUSBMUL + 1)
     PLLUSBo = 48 MHz = 16 MHz * (2 + 1) */
	MDR_RST_CLK->PLL_CONTROL |= (2 << RST_CLK_PLL_CONTROL_PLL_USB_MUL_Pos);
	
	/* Включение PLLUSB */
	MDR_RST_CLK->PLL_CONTROL |= RST_CLK_PLL_CONTROL_PLL_USB_ON;
	
	/* Пока блок PLLUSB не готов */
	while(!(MDR_RST_CLK->CLOCK_STATUS & RST_CLK_CLOCK_STATUS_PLL_USB_RDY));
	
	/* Сброс контроллера USB, >= 10 циклов тактовой частоты */
	MDR_USB->HSCR |= USB_HSCR_RESET_CORE;
	for(uint8_t i = 0; i < 10; i ++) {
		__asm { NOP }
	}
	MDR_USB->HSCR &= ~(1 << USB_HSCR_RESET_CORE_Pos);
	
	/* Режим HOST, DP, DM = PULLDOWN, Full Speed, 12 Mbps */
	MDR_USB->HSCR  |= USB_HSCR_HOST_MODE;
	MDR_USB->HSCR  |= USB_HSCR_EN_TX | USB_HSCR_EN_RX;
	MDR_USB->HSCR  |= USB_HSCR_DP_PULLDOWN | USB_HSCR_DM_PULLDOWN;
	MDR_USB->HTXLC |= USB_HTXLC_FSPL_Full | USB_HTXLC_FSLR_12Mb;
	
	/* Маска прерываний хоста:
	   разрешить все прерывания: SOF, TDONE, CONEV, RESUME */
	USB_SetHIM(USB_HIM_SET_MASK);
	NVIC_EnableIRQ(USB_IRQn);	
	host_st = HOST_ST_WAIT_CONN;
}

/* Инициализация хостом подключенного устройства */
void host_process(void) {
	switch(host_st) {
	case HOST_ST_WAIT_CONN: {
			if(dev_conn_st == DEV_CONN_ST_ATTACHED) {
				host_st = HOST_ST_CONN;
			}
		}
		break;
	case HOST_ST_CONN: {
			//sys_tim_LSI();
			//delay_ticks(4000); /* 100 ms */
			bus_rst();
			token = TOKEN_TYPE_OUT_DATA0;
			USB_SetHTXSE(USB_HTXSE_SOFEN_Auto);		
			ctrl_tsf_setup(0, (uint8_t *)&SETUP_PCK_GET_DESC, data_ptr, 8, 1, CTRL_TSF_TYPE_READ);
			bus_rst();
			host_st = HOST_ST_WAIT_DESC;
		}	
		break;
	case HOST_ST_WAIT_DESC: {
			if(ctrl_tsf() == CTRL_TSF_COMPLETE) {
				ctrl_tsf_setup(0, (uint8_t *)&SETUP_PCK_SET_ADDR, NULL, 0, 0, CTRL_TSF_TYPE_WRITE);
				host_st = HOST_ST_SET_ADDR;
			}
		}
		break;
	case HOST_ST_SET_ADDR: {
			if(ctrl_tsf() == CTRL_TSF_COMPLETE) {
				/* Чтение стандартного дескриптора в переменную desc */
				ctrl_tsf_setup(MASS_STORAGE_DEVICE_ADDRESS, (uint8_t *)&SETUP_PCK_GET_DESC, (uint8_t *)&desc, DEV_DESC_LEN, 1, CTRL_TSF_TYPE_READ);
				host_st = HOST_ST_WAIT_FULL_DESC;
			}
		}
		break;
	case HOST_ST_WAIT_FULL_DESC: {
			if(ctrl_tsf() == CTRL_TSF_COMPLETE) {
				/* Чтение остальных дескрипторов в переменную desc начиная с позиции cfg_bLength */
				ctrl_tsf_setup(MASS_STORAGE_DEVICE_ADDRESS, (uint8_t *)&SETUP_PCK_GET_DESC_CFG, (uint8_t *)&(desc.cfg_bLength),
													  (CFG_DESC_LEN + ITF_DESC_LEN + 2 * EP_DESC_LEN), 1, CTRL_TSF_TYPE_READ);
				host_st = HOST_ST_WAIT_FULL_DESC_CFG;
			}
		}
		break;
	case HOST_ST_WAIT_FULL_DESC_CFG: {
			if(ctrl_tsf() == CTRL_TSF_COMPLETE) {
				enumeration();
				/* После того как дескриптор конфигурации считан, хост посылает команду SetConfiguration с ненулевым значением, 
				   которое соответствует bConfigurationValue одной из конфигураций. Это используется для выбора нужной конфигурации. */
				SETUP_PCK_SET_CFG.wValue = desc.cfg_bConfigurationValue;
				ctrl_tsf_setup(MASS_STORAGE_DEVICE_ADDRESS, (uint8_t *)&SETUP_PCK_SET_CFG, NULL, 0, 0, CTRL_TSF_TYPE_WRITE);
				host_st = HOST_ST_SET_CFG;
			}
		}
		break;			
	case HOST_ST_SET_CFG: {
			if(ctrl_tsf() == CTRL_TSF_COMPLETE) {
				SETUP_PCK_SET_ITF.wValue = desc.itf_bAlternateSetting;
				SETUP_PCK_SET_ITF.wIndex = desc.itf_bInterfaceNumber;
				ctrl_tsf_setup(MASS_STORAGE_DEVICE_ADDRESS, (uint8_t *)&SETUP_PCK_SET_ITF, NULL, 0, 0, CTRL_TSF_TYPE_WRITE);
				host_st = HOST_ST_SET_ITF;
			}
		}
		break;
	case HOST_ST_SET_ITF: {
			if(ctrl_tsf() == CTRL_TSF_COMPLETE) {
				ctrl_tsf_setup(MASS_STORAGE_DEVICE_ADDRESS, (uint8_t *)&BULK_ONLY_MASS_STORAGE_RESET, NULL, 0, 0, CTRL_TSF_TYPE_WRITE);
				host_st = HOST_ST_BOT_RST;
			}
		}
		break;	
	case HOST_ST_BOT_RST:
			if(ctrl_tsf()== CTRL_TSF_COMPLETE){
				host_st = HOST_ST_CMPL;
				//rd_f_into_ufd();
			}
			break;
	case HOST_ST_CMPL:
			if(dev_conn_st == DEV_CONN_ST_DETACHED){
				host_st = HOST_ST_WAIT_CONN;
			}
			break;	
	}
}


/* Сброс шины USB */
void bus_rst(void) {
	USB_SetHTXLC(USB_HTXLC_TXLC_DM_Reset); 
	USB_SetHTXLC(USB_HTXLC_TXLC_DP_Reset); 
	USB_SetHTXLC(USB_HTXLC_DC_Direct); 
 // sys_tim_LSI();
	//delay_ticks(1200); /* 30 ms */
	USB_SetHTXLC(USB_HTXLC_DC_Normal);
}


/* Подготовка пакета к передаче */
void pck_tsf_setup(
	uint8_t dev_addr, 
	uint8_t ep_addr, 
	token_type_t 	token) 
{
	MDR_USB->HTXFC = 1; 			/* Очистить очередь буфера передачи */
	MDR_USB->HRXFC = 1;				/* Очистить очередь буфера приема */
	USB_SetHTXA(dev_addr);  	/* Адрес устройства */
	USB_SetHTXE(ep_addr); 		/* Номер оконечной точки */
	USB_SetHTXT(token); 			/* Тип транзакции */		
}


void enumeration(void) {
	if((desc.dev_bLength == DEV_DESC_LEN) &
		 (desc.cfg_bLength == CFG_DESC_LEN) &
		 (desc.itf_bLength == ITF_DESC_LEN) &
		 (desc.itf_bInterfaceSubClass == SCSI_TRANSPARENT_COMMAND_SET_INTERFACE_SUBCLASS_CODE)) {	
			 
		if(desc.itf_bNumEndpoints == MASS_STORAGE_DEVICE_EXPECTED_ENDPOINTS_NUMBER){
			if ((desc.ep_n1_bEndpointAddress & (1 << EP_DIR_Pos)) == EP_DIR_IN) {
				ep_in = desc.ep_n1_bEndpointAddress & EP_NUM_Msk;
				ep_out = desc.ep_n2_bEndpointAddress & EP_NUM_Msk;
			}
			else{
				ep_out = desc.ep_n1_bEndpointAddress & EP_NUM_Msk;	
				ep_in = desc.ep_n2_bEndpointAddress & EP_NUM_Msk;
			}
		}	
	}		 
}


void USB_IRQHandler(void) {
	/* Подключение / отключение */
	if(USB_HOST_EVENTS & USB_HIS_CONEV) {		
		if(USB_BUS_STATE == DISCONNECT) {
			dev_conn_st = DEV_CONN_ST_DETACHED; 
		}
		else if((USB_BUS_STATE == LOW_SPEED_CONNECT) || (USB_BUS_STATE == FULL_SPEED_CONNECT)) {
			dev_conn_st = DEV_CONN_ST_ATTACHED; 
		}
		USB_HOST_EVENTS |= USB_HIS_CONEV; 
	}
	
	/* Транзакция завершена */
	if(USB_HOST_EVENTS & USB_HIS_TDONE)
	{
		USB_HOST_EVENTS |= USB_HIS_TDONE;
		if(USB_HOST_RX_FIFO_STATE & USB_HRXS_ACKRXED) {
			ans = TRANS_ANS_ACK;
		}
		else if(USB_HOST_RX_FIFO_STATE & USB_HRXS_NAKRXED) {
			ans = TRANS_ANS_NAK;
		}
		else if(USB_HOST_RX_FIFO_STATE & USB_HRXS_STALLRXED) {
			ans = TRANS_ANS_STALL; 
		}
		else if(USB_HOST_RX_FIFO_STATE & USB_HRXS_RXTO) {
			ans = TRANS_ANS_TIMEOUT; 
		}
		else if(USB_HOST_RX_FIFO_STATE & USB_HRXS_CRCERR) {
			ans = TRANS_ANS_CRC_ERROR;
		}
		else if(USB_HOST_RX_FIFO_STATE & USB_HRXS_BSERR) {
			ans = TRANS_ANS_STUFF_ERROR;
		}
		else if(USB_HOST_RX_FIFO_STATE & USB_HRXS_RXOF) {
			ans = TRANS_ANS_FIFO_OVERFLOW; 
		}
		else if(USB_HOST_RX_FIFO_STATE & USB_HRXS_DATASEQ) {
			ans = TRANS_ANS_DATA1; 
		}
		else {
			ans = TRANS_ANS_DATA0; 
		}
	}
	
	/* Повтор */
	if(USB_HOST_EVENTS & USB_HIS_RESUME) { 
		USB_HOST_EVENTS |= USB_HIS_RESUME; 
	}
	
	/* SOF отправлен */
	if(USB_HOST_EVENTS & USB_HIS_SOFS) {
		USB_HOST_EVENTS |= USB_HIS_SOFS; 
	}
}


void rd_f_into_ufd(void) {	
	FATFS	 			 fs; 		 						/* Объект файловой системы */
	const TCHAR* path = "0";				/* Номер логического диска */
	BYTE         opt = 1;						/* Опция инициализации: 0 - отложенное монтирование
																												  1 - немедленное монтирование и проверка на доступность тома FAT */
	FRESULT 		 fr;    						/* Код результата операции */
	FIL 				 fil;								/* Файловый объект */
  UINT 				 bw = 0; 						/* Количество записанных байт */

	fr = f_mount(&fs, path, opt);
	fr = f_open(&fil, "session1.hex", FA_CREATE_NEW | FA_WRITE);
	
	uint8_t *ptr;
	while(r_addr < w_addr) {
		uint32_t tH = f_rd_tH(r_addr, MAIN);
		ptr = (uint8_t *)&tH;
		f_write(&fil, ptr, sizeof(uint32_t), &bw);
		r_addr += sizeof(uint32_t);
	}
	r_addr = 0;
	w_addr = 0;
	f_close(&fil);
}
