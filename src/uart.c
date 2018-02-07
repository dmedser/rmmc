#include "uart.h"
#include "flash.h"
#include "MDR32F9Qx_port.h"             // Milandr::Drivers:PORT
#include "MDR32F9Qx_rst_clk.h"          // Milandr::Drivers:RST_CLK
#include "MDR32F9Qx_uart.h"             // Milandr::Drivers:UART
#include <stdbool.h>

/* Буфер приема данных по UART */
uint32_t UART_RX_BUF = 0; 

/* Счетчик принятых по UART байт */
uint8_t  rx_cnt = 0;

/* Флаг получения внешней команды по UART */
bool 		 ext_cmd_received = false;

/* Код операции и параметр внешней команды */
uint8_t  ext_cmd_opcode;
uint16_t ext_cmd_param;


void uart_init(void) {
	/* Разрешение тактирования порта А */
	RST_CLK_PCLKcmd(RST_CLK_PCLK_PORTA, ENABLE);
	PORT_InitTypeDef pa;
	
	pa.PORT_MODE 			= PORT_MODE_DIGITAL;	
  pa.PORT_FUNC 			= PORT_FUNC_OVERRID;
  pa.PORT_SPEED 		= PORT_SPEED_MAXFAST;
  
  /* UART1_RX */
  pa.PORT_OE 	= PORT_OE_IN;
  pa.PORT_Pin = PORT_Pin_6;
  PORT_Init(MDR_PORTA, &pa);
	
	/* UART1_TX */
  pa.PORT_OE 	= PORT_OE_OUT;
  pa.PORT_Pin = PORT_Pin_7;
  PORT_Init(MDR_PORTA, &pa);

  /* Разрешение тактирования UART1 */
  RST_CLK_PCLKcmd(RST_CLK_PCLK_UART1, ENABLE);

  /* UART1_CLK = HCLK = 72 МГц */
  UART_BRGInit(MDR_UART1, UART_HCLKdiv1);
  
  NVIC_EnableIRQ(UART1_IRQn);

	UART_InitTypeDef uart1;
  uart1.UART_BaudRate                = 115000;
  uart1.UART_WordLength              = UART_WordLength8b;
  uart1.UART_StopBits                = UART_StopBits1;
  uart1.UART_Parity                  = UART_Parity_No;
	/* Запрещение работы FIFO буфера означает что прием данных производится побайтно,
   * прерывание "буфер приема заполнен" возникает на каждый принятый байт 
 	 */
  uart1.UART_FIFOMode                = UART_FIFO_OFF;
	/* Разрешение приема и передачи */
  uart1.UART_HardwareFlowControl     = UART_HardwareFlowControl_RXE | UART_HardwareFlowControl_TXE;
	
  UART_Init (MDR_UART1, &uart1);

  /* Разрешение RX прерываний от UART1 */
  UART_ITConfig(MDR_UART1, UART_IT_RX, ENABLE);

  /* Разрешение работы контроллера UART1 */
  UART_Cmd(MDR_UART1, ENABLE);
}


void UART1_IRQHandler(void) {
	if(UART_GetITStatus(MDR_UART1, UART_IT_RX) == SET) { /* Принят байт данных */
		rx_cnt++;
		UART_RX_BUF |= (uint32_t)(UART_ReceiveData(MDR_UART1) & 0x00FF);
		UART_RX_BUF = UART_RX_BUF << 8;
		if(rx_cnt == 3) {
			ext_cmd_received = true;
			ext_cmd_opcode = (uint8_t)(UART_RX_BUF >> 16);
			ext_cmd_param  = (uint16_t)(UART_RX_BUF & 0x0000FFFF);
			rx_cnt = 0;
			UART_RX_BUF = 0;
		}
	}
	UART_ClearITPendingBit(MDR_UART1, UART_IT_RX | UART_IT_TX);
}


void u_wr_byte(uint8_t data) {
	/* Check TXFE flag */
	while(UART_GetFlagStatus (MDR_UART1, UART_FLAG_TXFE)!= SET);
	UART_SendData(MDR_UART1, (uint16_t)data);
}


void u_wr_buf(uint8_t *buf, uint8_t len) {
	for(uint8_t i = 0; i < len; i++) {
		u_wr_byte(*buf++);
	}
}


void u_wr_tH(uint32_t tH) {
	uint8_t *ptr = (uint8_t *)&tH;
	u_wr_buf(ptr, sizeof(uint32_t));
}	


void rd_f_into_u(void) {
	while(r_addr < w_addr) {
		uint32_t tH = f_rd_tH(r_addr, MAIN);
		u_wr_tH(tH);
		r_addr += sizeof(uint32_t);
	}
	r_addr = 0;
	w_addr = 0;
}
