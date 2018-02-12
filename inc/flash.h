#include <stdint.h>
#include "MDR32Fx.h"

/* ������ ������ � ������ �� ���� */
extern uint32_t w_addr;
extern uint32_t r_addr;

/* ���� ���������/���������� ��� */
#define MAIN			0
#define RESERVE		1

/* ����� ������� ������ ������ � ������ ����� */
#define PROGRAM_BYTE_BUF_LEN			 	5
#define READ_BYTE_BUF_LEN 				  4

#define NUMBER_OF_SECTORS						8

/* ������ SPI 1 � 2 */
/* PORT F */
#define MOSI1			PORT_Pin_0
#define SCK1			PORT_Pin_1
#define nCS1			PORT_Pin_2
#define MISO1			PORT_Pin_3
/* PORT D */
#define MISO2			PORT_Pin_2
#define nCS2			PORT_Pin_3
#define SCK2			PORT_Pin_5
#define MOSI2			PORT_Pin_6

/* ������������� SPI ��� ����� � K1636��4� */
void 			spi_init(void);

void 			ncs1_l(void);
void 			ncs1_h(void);

/* ���� ������ SPI 1*/
uint8_t 	spi1_tsf(uint8_t byte);

/* �������� ������ � ����� ������ */
void      spi1_wr_byte(uint8_t data);
void 			spi1_wr_buf(uint8_t *buf, uint8_t len);

/* ����� ����� � ����� ������ */
uint8_t 	spi1_rd_byte(void);

void 			ncs2_l(void);
void 			ncs2_h(void);

uint8_t 	spi2_tsf(uint8_t byte);
void      spi2_wr_byte(uint8_t data);
void 			spi2_wr_buf(uint8_t *buf, uint8_t len);
uint8_t 	spi2_rd_byte(void);


/* ID ������������ � ���������� �1636��4� */
#define VendorID															(uint16_t)0x0100
#define DeviceID															(uint16_t)0x00C8

/* ���� �������� � �1636��4� */
#define ReadArraySlow													0x03
#define ReadArrayFast													0x0B
#define SectorErase														0xD8
#define ChipErase															0x60
#define ByteProgram														0x02
#define WriteEnable														0x06
#define WriteDisable													0x04
#define ProtectSector													0x36
#define UnprotectSector												0x39
#define ReadSectorProtectionRegister					0x3C
#define ReadStatusRegister										0x05
#define WriteStatusRegister										0x01
#define Reset																	0xF0
#define ReadVendorAndDeviceID									0x9F


/* �������������� ������� ������/������ ������ ��/�� ���� �� SPI */
void			f_wr_en(uint8_t prom_code, FunctionalState st);
void 			f_wr_byte(uint32_t addr, uint8_t data, uint8_t prom_code);
void 			f_wr_buf(uint8_t *buf, uint8_t len, uint32_t addr, uint8_t prom_code);
uint8_t 	f_rd_byte(uint32_t addr, uint8_t prom_code);

/* ��������� */
/* ������ ID ������������� � ���������� ���� ������ */
uint16_t  f_rd_id(uint8_t prom_code);

/* ������ ���������� ���� �������� ������ */
void 		 	f_unprotect(uint8_t prom_code);

/* �������� ���� ������ */
void 		 	f_erase(uint8_t prom_code);

/* ������������� */
void 		 	f_init(uint8_t prom_code);

/* ������ �� ���� �������� ����������� � ��������������� ������������ �������� */
void 			f_wr_t(uint32_t t, uint32_t addr, uint8_t prom_code);
/* ������ ��������������� �������� ����������� */
uint32_t 	f_rd_tH(uint32_t addr, uint8_t prom_code);
