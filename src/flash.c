#include "flash.h"
#include "hamming.h"
#include "MDR32Fx.h"                    // Device header
#include "MDR32F9Qx_rst_clk.h"          // Milandr::Drivers:RST_CLK
#include "MDR32F9Qx_port.h"             // Milandr::Drivers:PORT
#include "MDR32F9Qx_ssp.h"              // Milandr::Drivers:SSP
#include "util.h"

/* Адреса чтения и записи во флеш */
uint32_t w_addr;
uint32_t r_addr;

void spi_init(void) {
	/* Настройка портов F и D на режим работы по SPI */
	MDR_RST_CLK->PER_CLOCK |= RST_CLK_PER_CLOCK_PCLK_EN_PORTD | RST_CLK_PER_CLOCK_PCLK_EN_PORTF;
	PORT_InitTypeDef pf;
  PORT_DeInit(MDR_PORTF);
	/* Configure SSP1 pins: FSS, CLK, RXD, TXD */
  /* Configure PORTF pins 0, 1, 2, 3 */
	pf.PORT_Pin   = MISO1;
  pf.PORT_OE    = PORT_OE_IN;
  PORT_Init(MDR_PORTF, &pf);
  pf.PORT_Pin   = (MOSI1 | SCK1 | nCS1);
  pf.PORT_OE    = PORT_OE_OUT;
	pf.PORT_FUNC  = PORT_FUNC_ALTER;
  pf.PORT_MODE  = PORT_MODE_DIGITAL;
  pf.PORT_SPEED = PORT_SPEED_FAST;
  PORT_Init(MDR_PORTF, &pf);
	
	PORT_InitTypeDef pd;
	PORT_DeInit(MDR_PORTD);
	/* Configure SSP2 pins: FSS, CLK, RXD, TXD */
  /* Configure PORTD pins 2, 3, 5, 6 */
  pd.PORT_Pin   = MISO2;
	pd.PORT_OE    = PORT_OE_IN;
  PORT_Init(MDR_PORTD, &pd);
	pd.PORT_Pin   = (MOSI2 | SCK2 | nCS2);
  pd.PORT_OE    = PORT_OE_OUT;
  pd.PORT_FUNC  = PORT_FUNC_ALTER;
  pd.PORT_MODE  = PORT_MODE_DIGITAL;
  pd.PORT_SPEED = PORT_SPEED_FAST;
  PORT_Init(MDR_PORTD, &pd);

	/* Настройка SPI */
	MDR_RST_CLK->PER_CLOCK |= RST_CLK_PER_CLOCK_PCLK_EN_SPI1 | RST_CLK_PER_CLOCK_PCLK_EN_SPI2;
	/* Выбор источника сигнала для SSP_CLK = HCLK = 72 МГц*/
  SSP_BRGInit(MDR_SSP1, SSP_HCLKdiv1);
  SSP_BRGInit(MDR_SSP2, SSP_HCLKdiv1);
 
	SSP_InitTypeDef spi1;
	/* Делитель для SSP_CLK = 36, SSP_CLK = 2 МГц */
  spi1.SSP_SCR  = 0;
  spi1.SSP_CPSDVSR = 36;
  spi1.SSP_Mode = SSP_ModeMaster;
  spi1.SSP_WordLength = SSP_WordLength8b;
  spi1.SSP_SPH = SSP_SPH_1Edge;
  spi1.SSP_SPO = SSP_SPO_Low;
  spi1.SSP_FRF = SSP_FRF_SPI_Motorola;
 
  SSP_Init(MDR_SSP1, &spi1);
	SSP_Init(MDR_SSP2, &spi1);
	
	/* Enable SSP1 */
  SSP_Cmd(MDR_SSP1, ENABLE);
  /* Enable SSP2 */
  SSP_Cmd(MDR_SSP2, ENABLE);
}


uint8_t spi1_tsf(uint8_t data) {
	/* Пока буфер передачи не пуст */	
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_TFE) == RESET);
	SSP_SendData(MDR_SSP1, data);
	/* Пока буфер приема пуст */
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_RNE) == RESET);
	return (uint8_t)SSP_ReceiveData(MDR_SSP1);
} 


void spi1_wr_byte(uint8_t data) {
	 spi1_tsf(data);
}


void spi1_wr_buf(uint8_t *buf, uint8_t len) {
	for(uint8_t i = 0; i < len; i++) {
		spi1_wr_byte(*buf++);
	}
}


uint8_t spi1_rd_byte(void) {
	return spi1_tsf(0xFF); 
}


uint8_t spi2_tsf(uint8_t byte) {
	/* Wait for SPI2 Tx buffer empty */	
	while(SSP_GetFlagStatus(MDR_SSP2, SSP_FLAG_TFE) == RESET);
	SSP_SendData(MDR_SSP2, byte);
	/* Read SPI2 received data */
	while(SSP_GetFlagStatus(MDR_SSP2, SSP_FLAG_RNE) == RESET);
	return (uint8_t)SSP_ReceiveData(MDR_SSP2);
} 


void spi2_wr_byte(uint8_t byte) {
	 spi2_tsf(byte);
}


void spi2_wr_buf(uint8_t *buf, uint8_t len) {
	for(uint8_t i = 0; i < len; i++) {
		spi2_wr_byte(*buf++);
	}
}


uint8_t spi2_rd_byte(void) {
	return spi2_tsf(0xFF); 
}


uint16_t f_rd_id(uint8_t prom_code) {
	uint16_t vID;
	uint16_t dID;
	if(prom_code == MAIN) {
		spi1_wr_byte(ReadVendorAndDeviceID);
		vID = ((uint16_t)spi1_rd_byte()) << 8;
		dID = ((uint16_t)spi1_rd_byte());
	}
	else {
		spi2_wr_byte(ReadVendorAndDeviceID);
		vID = ((uint16_t)spi2_rd_byte()) << 8;
		dID = ((uint16_t)spi2_rd_byte());
	}
	return (vID | dID);
}



void f_unprotect(uint8_t prom_code) {
	uint32_t pck = ((uint32_t)UnprotectSector << 24);
	uint8_t *ptr = (uint8_t *)&pck;
	if(prom_code == MAIN) {
		spi1_wr_byte(WriteEnable);
		for(uint32_t i = 0; i < NUMBER_OF_SECTORS; i++) {
			pck &= 0xFF000000;
			pck |= (i << 18);				
			spi1_wr_buf(ptr, sizeof(uint32_t));
		}
		spi1_wr_byte(WriteDisable);
	}
	else {
		spi2_wr_byte(WriteEnable);
		pck = ((uint32_t)UnprotectSector << 24);
		ptr = (uint8_t *)&pck;
		for(uint32_t i = 0; i < NUMBER_OF_SECTORS; i++) {
			pck &= 0xFF000000;
			pck |= (i << 18);
			spi2_wr_buf(ptr, sizeof(uint32_t));
		}
		spi2_wr_byte(WriteDisable);
	}
}


void f_erase(uint8_t prom_code) {
	if(prom_code == MAIN) {
		spi1_wr_byte(WriteEnable);
		spi1_wr_byte(ChipErase);
		sys_tim_LSI();
		delay_ticks(80000); /* 2 c */
		spi1_wr_byte(WriteDisable);
	}
	else {	
		spi2_wr_byte(WriteEnable);
		spi2_wr_byte(ChipErase);
		sys_tim_LSI();
		delay_ticks(80000); /* 2 c */
		spi2_wr_byte(WriteDisable);
	}
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


/* Коды основного/резервного ПЗУ */
#define MAIN			0
#define RESERVE		1

/* Длины пакетов команд записи и чтения байта */
#define PROGRAM_BYTE_BUF_LEN			 	5
#define READ_BYTE_BUF_LEN 				  4


void f_wr_byte(uint32_t addr, uint8_t data, uint8_t prom_code) {
	uint8_t buf[PROGRAM_BYTE_BUF_LEN] = {ByteProgram, (uint8_t)(addr >> 16), (uint8_t)(addr >> 8), (uint8_t)addr, data};
	uint8_t *ptr = &buf[0];
	if(addr <= 0x1FFFFF) {
		if(prom_code == MAIN) {
			spi1_wr_byte(WriteEnable);
			spi1_wr_buf(ptr, PROGRAM_BYTE_BUF_LEN);
			spi1_wr_byte(WriteDisable);
		}
		else {
			spi2_wr_byte(WriteEnable);
			spi2_wr_buf(ptr, PROGRAM_BYTE_BUF_LEN);
			spi2_wr_byte(WriteDisable);
		}
	}
	sys_tim_HCLK();
	delay_ticks(14400); /* 200 мкс */
}


void f_wr_buf(uint8_t *buf, uint8_t len, uint32_t addr, uint8_t prom_code) {
	for(uint8_t i = 0; i < len; i++) {
		f_wr_byte(addr++, *buf++, prom_code);
	}
}


void f_wr_t(uint32_t t, uint32_t addr, uint8_t prom_code) {
	uint32_t tH = Hamming(t);
	uint8_t *ptr = (uint8_t *)&tH;
	f_wr_buf(ptr, sizeof(uint32_t), addr, prom_code);
}


uint8_t f_rd_byte(uint32_t addr, uint8_t prom_code) {
	uint8_t res = 0;
	uint8_t buf[READ_BYTE_BUF_LEN] = {ReadArraySlow, (uint8_t)(addr >> 16), (uint8_t)(addr >> 8), (uint8_t)addr};
	uint8_t *ptr = &buf[0];
	if(addr <= 0x1FFFFF) {
		if(prom_code == MAIN) {
			spi1_wr_byte(WriteEnable);
			spi1_wr_buf(ptr, READ_BYTE_BUF_LEN);
			res = spi1_rd_byte();
			spi1_wr_byte(WriteDisable);
		}
		else {
			spi2_wr_byte(WriteEnable);
			spi2_wr_buf(ptr, READ_BYTE_BUF_LEN);
			res = spi2_rd_byte();
			spi2_wr_byte(WriteDisable);
		}
	}
	return res;
}


uint32_t f_rd_tH(uint32_t addr, uint8_t prom_code) {
	uint32_t res = 0;
	for(uint8_t i = 0; i < sizeof(uint32_t); i++) {
		res |= (f_rd_byte(addr++, prom_code) << ((2 - i) * 8));
	}
	return res;
}
