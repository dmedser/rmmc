#include "flash.h"
#include "hamming.h"
#include "MDR32Fx.h"                    // Device header
#include "MDR32F9Qx_rst_clk.h"          // Milandr::Drivers:RST_CLK
#include "MDR32F9Qx_port.h"             // Milandr::Drivers:PORT
#include "MDR32F9Qx_ssp.h"              // Milandr::Drivers:SSP
#include "util.h"
#include "MDR32F9Qx_timer.h"            // Milandr::Drivers:TIMER


/* Адреса чтения и записи во флеш */
uint32_t w_addr;
uint32_t r_addr;

void spi_init(void) {
	/* Настройка портов F и D на режим работы по SPI */
	MDR_RST_CLK->PER_CLOCK |= RST_CLK_PER_CLOCK_PCLK_EN_PORTF; //| RST_CLK_PER_CLOCK_PCLK_EN_PORTD;
	
	/************** PORT F SPI 1 **************/
	PORT_DeInit(MDR_PORTF);
//	/* PF0 TXD(MOSI1), PF1 CLK(SCK1) */
//	PORT_InitTypeDef pf01;
//  pf01.PORT_Pin   = MOSI1 | SCK1;  
//  pf01.PORT_OE    = PORT_OE_OUT;
//	pf01.PORT_MODE  = PORT_MODE_DIGITAL;
//	pf01.PORT_FUNC  = PORT_FUNC_ALTER;
//  pf01.PORT_SPEED = PORT_SPEED_FAST;
//  PORT_Init(MDR_PORTF, &pf01);	
//	
	/* PF2 FSS(nCS1) */
	PORT_InitTypeDef pf2;
	pf2.PORT_Pin   = nCS1;
  pf2.PORT_OE    = PORT_OE_OUT;
	pf2.PORT_MODE  = PORT_MODE_DIGITAL;
	pf2.PORT_FUNC  = PORT_FUNC_PORT;
  pf2.PORT_SPEED = PORT_SPEED_FAST;
  PORT_Init(MDR_PORTF, &pf2);
	
//	/* PF3 RXD(MISO1) */
//	PORT_InitTypeDef pf3;
//	pf3.PORT_Pin   = MISO1;
//  pf3.PORT_OE    = PORT_OE_IN;
//	pf3.PORT_MODE  = PORT_MODE_DIGITAL;
//	pf3.PORT_FUNC  = PORT_FUNC_ALTER;
//  pf3.PORT_SPEED = PORT_SPEED_FAST;
//  PORT_Init(MDR_PORTF, &pf3);
	
//	/************** PORT D SPI 2 **************/
//	PORT_DeInit(MDR_PORTD);
//	/* PD2 RXD(MISO2) */
//	PORT_InitTypeDef pd2;
//  pd2.PORT_Pin   = MISO2;									
//	pd2.PORT_OE    = PORT_OE_IN;
//	pd2.PORT_MODE  = PORT_MODE_DIGITAL;
//	pd2.PORT_FUNC  = PORT_FUNC_ALTER;
//  pd2.PORT_SPEED = PORT_SPEED_FAST;
//  PORT_Init(MDR_PORTD, &pd2);
//	
//	/* PD3 FSS(nCS2) */
//	PORT_InitTypeDef pd3;
//	pd3.PORT_Pin   = nCS2;  
//  pd3.PORT_OE    = PORT_OE_OUT;
//  pd3.PORT_FUNC  = PORT_FUNC_PORT;
//  pd3.PORT_MODE  = PORT_MODE_DIGITAL;
//  pd3.PORT_SPEED = PORT_SPEED_FAST;
//  PORT_Init(MDR_PORTD, &pd3);
//	
//	/* PD5 CLK(SCK2), PD6 TXD(MOSI2) */
//	PORT_InitTypeDef pd56;
//	pd56.PORT_Pin   = SCK2 | MOSI2;  
//  pd56.PORT_OE    = PORT_OE_OUT;
//  pd56.PORT_FUNC  = PORT_FUNC_ALTER;
//  pd56.PORT_MODE  = PORT_MODE_DIGITAL;
//  pd56.PORT_SPEED = PORT_SPEED_FAST;
//  PORT_Init(MDR_PORTD, &pd56);
//	
	ncs1_h();
//  ncs2_h();
//	
//	/* Настройка SPI */
//	MDR_RST_CLK->PER_CLOCK |= RST_CLK_PER_CLOCK_PCLK_EN_SPI1 | RST_CLK_PER_CLOCK_PCLK_EN_SPI2;
//	/* Выбор источника сигнала для SSP_CLK = HCLK = 80 МГц*/
//  SSP_BRGInit(MDR_SSP1, SSP_HCLKdiv1);
//  SSP_BRGInit(MDR_SSP2, SSP_HCLKdiv1);
// 
//	SSP_InitTypeDef spi;
//  /* Частота тактового сигнала SPI = F_SSPCLK / (CPSDVR * (1  + SCR)), 
//	   где CPSDVR – четное число в диапазоне от 2 до 254, 
//		 а SCR – число от 0 до 255 */  
//		 
//	/* Делитель для SSP_CLK = 40, SSP_CLK = 2 МГц */
//  spi.SSP_SCR  			 = 0;
//  spi.SSP_CPSDVSR 	 = 40;
//  spi.SSP_Mode 			 = SSP_ModeMaster;
//  spi.SSP_WordLength = SSP_WordLength8b;
//  spi.SSP_SPH 			 = SSP_SPH_1Edge;
//  spi.SSP_SPO 			 = SSP_SPO_Low;
//  spi.SSP_FRF 			 = SSP_FRF_SPI_Motorola;
// 
//  SSP_Init(MDR_SSP1, &spi);
//	SSP_Init(MDR_SSP2, &spi);
//	
//	/* Enable SSP1 */
//  SSP_Cmd(MDR_SSP1, ENABLE);
//  /* Enable SSP2 */
//  SSP_Cmd(MDR_SSP2, ENABLE);
}


inline void ncs1_l(void) {
	MDR_PORTF->RXTX &= ~nCS1;
}


inline void ncs1_h(void) {
	MDR_PORTF->RXTX |= nCS1;
}


uint8_t spi1_tsf(uint8_t data) {
	/* Пока буфер передачи не пуст */	
	while((MDR_SSP1->SR & SSP_FLAG_TFE) == 0);
	MDR_SSP1->DR = data;
	/* Пока буфер приема пуст */
	while((MDR_SSP1->SR & SSP_FLAG_RNE) == 0);
	return (uint8_t)MDR_SSP1->DR;
} 


void spi1_wr_byte(uint8_t data) {
	spi1_tsf(data);
}


void spi1_wr_buf(uint8_t *buf, uint8_t len) {
	for(uint8_t i = 0; i < len; i++) {
		spi1_tsf(*buf++);
	}
}


uint8_t spi1_rd_byte(void) {
	return spi1_tsf(0xFF);
}


inline void ncs2_l(void) {
	MDR_PORTD->RXTX &= ~nCS2;
}


inline void ncs2_h(void) {
	MDR_PORTD->RXTX |= nCS2;
}


uint8_t spi2_tsf(uint8_t data) {
	/* Пока буфер передачи не пуст */	
	while((MDR_SSP2->SR & SSP_FLAG_TFE) == 0);
	MDR_SSP2->DR = data;
	/* Пока буфер приема пуст */
	while((MDR_SSP2->SR & SSP_FLAG_RNE) == 0);
	return (uint8_t)MDR_SSP2->DR;
} 


void spi2_wr_byte(uint8_t data) {
	spi2_tsf(data);
}


void spi2_wr_buf(uint8_t *buf, uint8_t len) {
	for(uint8_t i = 0; i < len; i++) {
		spi2_tsf(*buf++);
	}
}


uint8_t spi2_rd_byte(void) {
	return spi2_tsf(0xFF);
}


void f_wr_en(uint8_t prom_code, FunctionalState st) {
	uint8_t cmd = (st == ENABLE) ? WriteEnable : WriteDisable;
	if(prom_code == MAIN) {
		ncs1_l();
		spi1_wr_byte(cmd);
		ncs1_h();
	}
	else {
		ncs2_l();
		spi2_wr_byte(cmd);
		ncs2_h();
	}
}


void f_wr_byte(uint32_t addr, uint8_t data, uint8_t prom_code) {
	uint8_t buf[PROGRAM_BYTE_BUF_LEN] = {ByteProgram, (uint8_t)(addr >> 16), (uint8_t)(addr >> 8), (uint8_t)addr, data};
	uint8_t *ptr = &buf[0];
	if(addr <= 0x1FFFFF) {
		if(prom_code == MAIN) {			
			ncs1_l();
			spi1_wr_buf(ptr, PROGRAM_BYTE_BUF_LEN);
			ncs1_h();
		}
		else {
			ncs2_l();
			spi2_wr_buf(ptr, PROGRAM_BYTE_BUF_LEN);
			ncs2_h();
		}
	}
	delay_ms(1); /* >= 200 мкс */
}


void f_wr_buf(uint8_t *buf, uint8_t len, uint32_t addr, uint8_t prom_code) {
	f_wr_en(prom_code, ENABLE);
	delay_ms(1);
	for(uint8_t i = 0; i < len; i++) {
		f_wr_byte(addr++, *buf++, prom_code);
	}
	delay_ms(1);
	f_wr_en(prom_code, DISABLE);
}


uint8_t f_rd_byte(uint32_t addr, uint8_t prom_code) {
	uint8_t res = 0;
	uint8_t buf[READ_BYTE_BUF_LEN] = {ReadArraySlow, (uint8_t)(addr >> 16), (uint8_t)(addr >> 8), (uint8_t)addr};
	uint8_t *ptr = &buf[0];
	if(addr <= 0x1FFFFF) {
		if(prom_code == MAIN) {
			ncs1_l();
			spi1_wr_buf(ptr, READ_BYTE_BUF_LEN);
			res = spi1_rd_byte();
			ncs1_h();
		}
		else {
			ncs2_l();
			spi2_wr_buf(ptr, READ_BYTE_BUF_LEN);
			res = spi2_rd_byte();
			ncs2_h();
		}
	}
	return res;
}


uint16_t f_rd_id(uint8_t prom_code) {
	uint16_t vID;
	uint16_t dID;
	f_wr_en(prom_code, ENABLE);
	delay_ms(1);
	if(prom_code == MAIN) {
		ncs1_l();
		spi1_wr_byte(ReadVendorAndDeviceID);
		vID = ((uint16_t)spi1_rd_byte()) << 8;
		dID = ((uint16_t)spi1_rd_byte());
		ncs1_h();
	}
	else {
		ncs2_l();
		spi2_wr_byte(ReadVendorAndDeviceID);
		vID = ((uint16_t)spi2_rd_byte()) << 8;
		dID = ((uint16_t)spi2_rd_byte());
		ncs2_h();
	}
	delay_ms(1);
	f_wr_en(prom_code, DISABLE);
	return (vID | dID);
}


void f_unprotect(uint8_t prom_code) {
	uint32_t pck = ((uint32_t)UnprotectSector << 24);
	uint8_t *ptr = (uint8_t *)&pck;
	f_wr_en(prom_code, ENABLE);
	delay_ms(1);
	if(prom_code == MAIN) {		
		ncs1_l();
		for(uint32_t i = 0; i < NUMBER_OF_SECTORS; i++) {
			pck &= 0xFF000000;
			pck |= (i << 18);				
			spi1_wr_buf(ptr, sizeof(uint32_t));
		}
		ncs1_h();
	}
	else {
		ncs2_l();
		for(uint32_t i = 0; i < NUMBER_OF_SECTORS; i++) {
			pck &= 0xFF000000;
			pck |= (i << 18);
			spi2_wr_buf(ptr, sizeof(uint32_t));
		}
		ncs2_h();
	}
	delay_ms(1);
	f_wr_en(prom_code, DISABLE);
}


void f_erase(uint8_t prom_code) {
	f_wr_en(prom_code, ENABLE);
	delay_ms(1);
	if(prom_code == MAIN) {	
		ncs1_l();
		spi1_wr_byte(ChipErase);
		ncs1_h();
	}
	else {
		ncs2_l();
		spi2_wr_byte(ChipErase);
		ncs2_h();
	}
	delay_ms(2000); /* 2 c */
	f_wr_en(prom_code, DISABLE);
}


void f_init(uint8_t prom_code) {
	spi_init();
	uint16_t result_IDs 	= f_rd_id(prom_code);
	uint16_t required_IDs = (VendorID | DeviceID);
	if(result_IDs == required_IDs) {
		f_unprotect(prom_code);
		f_erase(prom_code);
	}
}


void f_wr_t(uint32_t t, uint32_t addr, uint8_t prom_code) {
	uint32_t tH = Hamming(t);
	uint8_t *ptr = (uint8_t *)&tH;
	f_wr_buf(ptr, sizeof(uint32_t), addr, prom_code);
}


uint32_t f_rd_tH(uint32_t addr, uint8_t prom_code) {
	uint32_t res = 0;
	f_wr_en(prom_code, ENABLE);
	delay_ms(1);
	for(uint8_t i = 0; i < sizeof(uint32_t); i++) {
		res |= (f_rd_byte(addr++, prom_code) << ((2 - i) * 8));
	}
	delay_ms(1);
	f_wr_en(prom_code, DISABLE);
	return res;
}
