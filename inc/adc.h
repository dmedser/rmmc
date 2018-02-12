#include <stdint.h>

/* Коды направления передачи данных */
typedef enum {
  IN			= 0x0,
  OUT			= 0x1
}DIR_TypeDef;

/* Коды логический уровней */
typedef enum {
  LOW			= 0x0,
  HIGH		= 0x1
}LVL_TypeDef;

/* Выводы АЦП */
#define SCLK  	PORT_Pin_5
#define nRFS  	PORT_Pin_6
#define nTFS  	PORT_Pin_7
#define A0    	PORT_Pin_8
#define nDRDY		PORT_Pin_9
#define SDATA 	PORT_Pin_10

/* Количество тактов в циклах записи/чтения данных в/из АЦП */
#define WR_TICKS_COUNT 		24
#define RD_TICKS_COUNT 		24

/* Поля регистра управления */
typedef struct {
	uint32_t MD;
	uint32_t G;
	uint32_t CH;
	uint32_t WL;
	uint32_t IO;
	uint32_t BO;
	uint32_t BU;
	uint32_t FS;
}ADC_CTRL_REG_TypeDef;

/* Смещения полей регистра управления */
#define MD_Pos 	21
#define G_Pos 	18
#define CH_Pos	17
#define WL_Pos 	15
#define IO_Pos 	14
#define BO_Pos 	13
#define BU_Pos 	12
#define FS_Pos	0

/* Маски полей регистра управления */
#define MD_Msk	((uint32_t)0x00E00000)
#define G_Msk 	((uint32_t)0x001C0000)
#define CH_Msk	((uint32_t)0x00020000)
#define WL_Msk 	((uint32_t)0x00008000)
#define IO_Msk 	((uint32_t)0x00004000)
#define BO_Msk 	((uint32_t)0x00002000)
#define BU_Msk 	((uint32_t)0x00001000)
#define FS_Msk	((uint32_t)0x00000FFF)

/* Значения полей регистра управления */
#define MD_NORM             (((uint32_t)0x0) << MD_Pos) 
#define MD_SYS_CLB_1        (((uint32_t)0x2) << MD_Pos) 
#define MD_SYS_CLB_2        (((uint32_t)0x3) << MD_Pos) 
#define MD_SCL_ZERO     	  (((uint32_t)0x6) << MD_Pos)
#define MD_SCL_FULL      		(((uint32_t)0x7) << MD_Pos)


#define G_x1             		(((uint32_t)0x0) << G_Pos) 
#define G_x2        				(((uint32_t)0x1) << G_Pos) 

#define CH_1             		(((uint32_t)0x0) << CH_Pos) 
#define CH_2        				(((uint32_t)0x1) << CH_Pos) 

#define WL_16             	(((uint32_t)0x0) << WL_Pos) 
#define WL_24        				(((uint32_t)0x1) << WL_Pos) 

#define IO_OFF             	(((uint32_t)0x0) << IO_Pos) 
#define IO_ON        				(((uint32_t)0x1) << IO_Pos) 

#define BO_OFF             	(((uint32_t)0x0) << BO_Pos) 
#define BO_ON        				(((uint32_t)0x1) << BO_Pos) 

#define BU_BPLR       		  (((uint32_t)0x0) << BU_Pos) 
#define BU_UPLR        		  (((uint32_t)0x1) << BU_Pos) 

/* Низкоуровневые функции */

/* Инициализация выводов АЦП, 
 * порт B, пины 5, 6, 7, 8, 9, 10 
 * PB5  - [OUT] 	 SCLK
 * PB6  - [OUT] 	 #RFS  
 * PB7  - [OUT] 	 #TFS
 * PB8  - [OUT] 	 A0
 * PB9  - [IN]  	 #DRDY
 * PB10 - [IN/OUT] SDATA
 */	
void port_b_init(void);	

/* Генерация тактового сигнала частотой 250 кГц
 * для программного интерфейса связи с АЦП 
 */
void tim_3_ch_3_init(void);									

///*  */
//void ref_on(void);
//void ref_off(void);

/* Задание направления пина порта ввода/вывода */
void set_dir(uint16_t pin, DIR_TypeDef dir);	

/* Задание логического уровня пина порта ввода/вывода */
void set_lvl(uint16_t pin, LVL_TypeDef lvl);

/* Цикл записи данных в АЦП */
void 		 adc_wr(uint32_t val);

/* Цикл чтения даных из АЦП */
uint32_t adc_rd(void);

/* Чтение регистра управления АЦП */
uint32_t adc_rd_cr(void);

/* Запись данных в регистр управления АЦП */
void 		 adc_wr_cr(uint32_t val);

/* Чтение регистра данных АЦП */
uint32_t adc_rd_dr(void);

/* Настройка регистра управления АЦП */
void 		 ADC_CTRL_REG_StructInit(ADC_CTRL_REG_TypeDef *cr);

/* Системная калибровка (нуля шкалы и полной шкалы) обоих каналов АЦП */
void 		 adc_sys_clb(void);


/* Интерфейс */

/* Инициализация АЦП */
void 		 adc_init(void);

/* Чтение 24-рязрядного выходного значения АЦП каналов 1 и 2 */
uint32_t adc_t1(void);
uint32_t adc_t2(void);
