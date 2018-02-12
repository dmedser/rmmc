#include <stdint.h>
#include "MDR32Fx.h"

/* Адреса чтения и записи во флеш */
extern uint32_t w_addr;
extern uint32_t r_addr;

/* Коды основного/резервного ПЗУ */
#define MAIN			0
#define RESERVE		1

/* Длины пакетов команд записи и чтения байта */
#define PROGRAM_BYTE_BUF_LEN			 	5
#define READ_BYTE_BUF_LEN 				  4

#define NUMBER_OF_SECTORS						8

/* Выводы SPI 1 и 2 */
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

/* Инициализация SPI для связи с K1636РР4У */
void 			spi_init(void);

void 			ncs1_l(void);
void 			ncs1_h(void);

/* Цикл обмена SPI 1*/
uint8_t 	spi1_tsf(uint8_t byte);

/* Отправка данных в цикле обмена */
void      spi1_wr_byte(uint8_t data);
void 			spi1_wr_buf(uint8_t *buf, uint8_t len);

/* Прием байта в цикле обмена */
uint8_t 	spi1_rd_byte(void);

void 			ncs2_l(void);
void 			ncs2_h(void);

uint8_t 	spi2_tsf(uint8_t byte);
void      spi2_wr_byte(uint8_t data);
void 			spi2_wr_buf(uint8_t *buf, uint8_t len);
uint8_t 	spi2_rd_byte(void);


/* ID проиводителя и устройства К1636РР4У */
#define VendorID															(uint16_t)0x0100
#define DeviceID															(uint16_t)0x00C8

/* Коды операций с К1636РР4У */
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


/* Низкоуровневые функции записи/чтения данных во/из флеш по SPI */
void			f_wr_en(uint8_t prom_code, FunctionalState st);
void 			f_wr_byte(uint32_t addr, uint8_t data, uint8_t prom_code);
void 			f_wr_buf(uint8_t *buf, uint8_t len, uint32_t addr, uint8_t prom_code);
uint8_t 	f_rd_byte(uint32_t addr, uint8_t prom_code);

/* Интерфейс */
/* Чтение ID производителя и устройства флеш памяти */
uint16_t  f_rd_id(uint8_t prom_code);

/* Снятие блокировки всех секторов памяти */
void 		 	f_unprotect(uint8_t prom_code);

/* Стирание всей памяти */
void 		 	f_erase(uint8_t prom_code);

/* Инициализация */
void 		 	f_init(uint8_t prom_code);

/* Запись во флеш значения температуры с предварительным кодированием Хэмминга */
void 			f_wr_t(uint32_t t, uint32_t addr, uint8_t prom_code);
/* Чтение закодированного значения температуры */
uint32_t 	f_rd_tH(uint32_t addr, uint8_t prom_code);
