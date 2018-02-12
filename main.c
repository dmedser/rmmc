#include "util.h"
#include "adc.h"
#include "flash.h"
#include "pid.h"
#include "uart.h"
#include "usb_host.h"


int main(void) { 
	cpu_init();
	//adc_init();
	sys_tim_init();
	carrier_init();
	//f_init(MAIN);
	carrier_cmd(ENABLE);
	
	while(1);
	
	//cpu_init();
	//adc_init();
	//f_init();
	//carrier_init();
	//uart_init();
	//usb_host_init();
	/*
	while(1) {
		if(dev_conn_st == DEV_CONN_ST_ATTACHED) {
			while(host_st != HOST_ST_CMPL) {
				host_process();
			}
		}
	*/
//		if(ext_cmd_received) {
//			switch(ext_cmd_opcode) {
//				case STATE:
//					carrier_cmd((FunctionalState)ext_cmd_param);
//					break;
//				case SET_TMP:
//					target_t = ext_cmd_param;
//					break;
//				case RD_INTO:
//					if(ext_cmd_param == UART) {
//						rd_f_into_u();
//					}
//					else if(ext_cmd_param == USB_FLASH_DRIVE) {
//						rd_f_into_ufd();
//					}
//					break;
//			}
//			ext_cmd_received = false;
//		}
//	}
}
