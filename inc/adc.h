#include <stdint.h>

/* ���� ����������� �������� ������ */
typedef enum {
  IN			= 0x0,
  OUT			= 0x1
}DIR_TypeDef;

/* ���� ���������� ������� */
typedef enum {
  LOW			= 0x0,
  HIGH		= 0x1
}LVL_TypeDef;

/* ������ ��� */
#define SCLK  	PORT_Pin_5
#define nRFS  	PORT_Pin_6
#define nTFS  	PORT_Pin_7
#define A0    	PORT_Pin_8
#define nDRDY		PORT_Pin_9
#define SDATA 	PORT_Pin_10

/* ���������� ������ � ������ ������/������ ������ �/�� ��� */
#define WR_TICKS_COUNT 		24
#define RD_TICKS_COUNT 		24

/* ���� �������� ���������� */
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

/* �������� ����� �������� ���������� */
#define MD_Pos 	21
#define G_Pos 	18
#define CH_Pos	17
#define WL_Pos 	15
#define IO_Pos 	14
#define BO_Pos 	13
#define BU_Pos 	12
#define FS_Pos	0

/* ����� ����� �������� ���������� */
#define MD_Msk	((uint32_t)0x00E00000)
#define G_Msk 	((uint32_t)0x001C0000)
#define CH_Msk	((uint32_t)0x00020000)
#define WL_Msk 	((uint32_t)0x00008000)
#define IO_Msk 	((uint32_t)0x00004000)
#define BO_Msk 	((uint32_t)0x00002000)
#define BU_Msk 	((uint32_t)0x00001000)
#define FS_Msk	((uint32_t)0x00000FFF)

/* �������� ����� �������� ���������� */
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

/* �������������� ������� */

/* ������������� ������� ���, 
 * ���� B, ���� 5, 6, 7, 8, 9, 10 
 * PB5  - [OUT] 	 SCLK
 * PB6  - [OUT] 	 #RFS  
 * PB7  - [OUT] 	 #TFS
 * PB8  - [OUT] 	 A0
 * PB9  - [IN]  	 #DRDY
 * PB10 - [IN/OUT] SDATA
 */	
void port_b_init(void);	

/* ��������� ��������� ������� �������� 250 ���
 * ��� ������������ ���������� ����� � ��� 
 */
void tim_3_ch_3_init(void);									

///*  */
//void ref_on(void);
//void ref_off(void);

/* ������� ����������� ���� ����� �����/������ */
void set_dir(uint16_t pin, DIR_TypeDef dir);	

/* ������� ����������� ������ ���� ����� �����/������ */
void set_lvl(uint16_t pin, LVL_TypeDef lvl);

/* ���� ������ ������ � ��� */
void 		 adc_wr(uint32_t val);

/* ���� ������ ����� �� ��� */
uint32_t adc_rd(void);

/* ������ �������� ���������� ��� */
uint32_t adc_rd_cr(void);

/* ������ ������ � ������� ���������� ��� */
void 		 adc_wr_cr(uint32_t val);

/* ������ �������� ������ ��� */
uint32_t adc_rd_dr(void);

/* ��������� �������� ���������� ��� */
void 		 ADC_CTRL_REG_StructInit(ADC_CTRL_REG_TypeDef *cr);

/* ��������� ���������� (���� ����� � ������ �����) ����� ������� ��� */
void 		 adc_sys_clb(void);


/* ��������� */

/* ������������� ��� */
void 		 adc_init(void);

/* ������ 24-���������� ��������� �������� ��� ������� 1 � 2 */
uint32_t adc_t1(void);
uint32_t adc_t2(void);
