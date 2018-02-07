#include "adc.h"
#include "util.h"
#include "MDR32Fx.h"                    // Device header
#include "MDR32F9Qx_rst_clk.h"          // Milandr::Drivers:RST_CLK
#include "MDR32F9Qx_port.h"             // Milandr::Drivers:PORT
#include "MDR32F9Qx_timer.h"            // Milandr::Drivers:TIMER
#include <stdbool.h>

/* Буферы приема/передачи данных АЦП */
uint32_t ADC_TX_BUF;
uint32_t ADC_RX_BUF;

/* Флаги завершения операций */
bool tx_cmpl = false;
bool rx_cmpl = false;

/* Счетчик тактов в цикле записи/чтения */
uint8_t ticks = 0;

void port_b_init(void) {
	MDR_RST_CLK->PER_CLOCK |= RST_CLK_PER_CLOCK_PCLK_EN_PORTB;
	
	/* ADC 1273PV9R */

	/***** OUTPUT *****/
	/* SCLK - TIM3 CH3 PWM */
	PORT_InitTypeDef pb5;
	pb5.PORT_Pin 	 = PORT_Pin_5;
	pb5.PORT_OE 	 = PORT_OE_OUT;
	pb5.PORT_MODE  = PORT_MODE_DIGITAL;
	pb5.PORT_FUNC  = PORT_FUNC_OVERRID;
	pb5.PORT_SPEED = PORT_SPEED_FAST; 	 	/* should be <= 50 ns */
	PORT_Init(MDR_PORTB, &pb5);
	
	/* #RFS & #TFS & A0 */
	PORT_InitTypeDef pb678;
	pb678.PORT_Pin 	 = PORT_Pin_6 | PORT_Pin_7 | PORT_Pin_8;
	pb678.PORT_OE 	 = PORT_OE_OUT;
	pb678.PORT_MODE  = PORT_MODE_DIGITAL;
	pb678.PORT_FUNC  = PORT_FUNC_PORT;
	pb678.PORT_SPEED = PORT_SPEED_FAST; 
	PORT_Init(MDR_PORTB, &pb678);
	
	/***** INPUT *****/
	/* #DRDY */
	PORT_InitTypeDef pb9;
	pb9.PORT_Pin 	 = PORT_Pin_9;
	pb9.PORT_OE 	 = PORT_OE_IN;
	pb9.PORT_MODE  = PORT_MODE_DIGITAL;
	pb9.PORT_FUNC  = PORT_FUNC_PORT;
	pb9.PORT_SPEED = PORT_SPEED_FAST; 
	PORT_Init(MDR_PORTB, &pb9);
	
	/* INPUT/OUTPUT */
	/* SDATA */
	PORT_InitTypeDef pb10;
	pb10.PORT_Pin 	= PORT_Pin_10;
	pb10.PORT_MODE  = PORT_MODE_DIGITAL;
	pb10.PORT_FUNC  = PORT_FUNC_PORT;
	pb10.PORT_SPEED = PORT_SPEED_FAST; 
	PORT_Init(MDR_PORTB, &pb10);
}

/* Таймер 250 кГц */
void tim_3_ch_3_init(void) {
	TIMER_DeInit(MDR_TIMER3);
	/* Разрешение тактирования TIM3 */
	MDR_RST_CLK->PER_CLOCK |= RST_CLK_PER_CLOCK_PCLK_EN_TIMER3;
  /* Настройка источника тактирования TIM3 */
	/* HCLK без предделения */
	MDR_RST_CLK->TIM_CLOCK |= (RST_CLK_TIM_CLOCK_TIM_BRG_HCLK << RST_CLK_TIM_CLOCK_TIM3_BRG_Pos);
	/* Разрешение тактирования TIM3 */
	MDR_RST_CLK->TIM_CLOCK |= RST_CLK_TIM_CLOCK_TIM3_CLK_EN;
	
	/* Обнуление счетчика */	
	MDR_TIMER3->CNT = 0;
	/* Без предделения */
	MDR_TIMER3->PSG = 0;
	/* Основание счета */
  MDR_TIMER3->ARR = 320 - 1;  /* 4 мкс */
	
	/* Направление счета - вверх от 0 до ARR */
	MDR_TIMER3->CNTRL &= ~(1 << TIMER_CNTRL_DIR_Pos); 
	MDR_TIMER3->CNTRL &= ~(0x3 << TIMER_CNTRL_CNT_MODE_Pos);
	
	/* Первый регистр сравнения */
	MDR_TIMER3->CCR3  = 160 - 1; /* 2 мкс */
  /* Второй регистр сравнения = ARR */
  MDR_TIMER3->CCR31 = 320 - 1; /* 4 мкс */
	
	/* Настройка канала */
	/* Без предделения */
	MDR_TIMER3->CH3_CNTRL &= ~(0x3 << TIMER_CH_CNTRL_CHPSC_Pos); 
	/* Формат выработки сигнала REF - переключение REF если CNT == CCR или CNT == CRR1 */
	MDR_TIMER3->CH3_CNTRL |= (TIMER_CH_CNTRL_OCCM_SW_REF_CNT_CCR_OR_CNT_CCR1 << TIMER_CH_CNTRL_OCCM_Pos);
	/* Режим работы - ШИМ */
	MDR_TIMER3->CH3_CNTRL &= ~(1 << TIMER_CH_CNTRL_CAP_NPWM_Pos);
	/* Прямой канал всегда работает на выход, на выход подается сигнал REF */
	MDR_TIMER3->CH3_CNTRL1 |= (TIMER_CH_CNTRL1_SELOE_OUT_EN << TIMER_CH_CNTRL1_SELOE_Pos);
	MDR_TIMER3->CH3_CNTRL1 |= (TIMER_CH_CNTRL1_SELO_OUT_REF << TIMER_CH_CNTRL1_SELO_Pos);
	/* Разрешение CRR1 */
	MDR_TIMER3->CH3_CNTRL2 |= TIMER_CH_CNTRL2_CCR1_EN;
}


void set_dir(uint16_t pin, DIR_TypeDef dir) {
	MDR_PORTB->OE = (dir == IN) ? (MDR_PORTB->OE & ~pin) : (MDR_PORTB->OE | pin);
}

void set_lvl(uint16_t pin, LVL_TypeDef lvl) {
	MDR_PORTB->RXTX = (lvl == LOW) ? (MDR_PORTB->RXTX & ~pin) : (MDR_PORTB->RXTX | pin); 
}


void adc_wr(uint32_t val) {
	ADC_TX_BUF = val << 8;
	MDR_PORTB->RXTX = (((ADC_TX_BUF >> 21) & SDATA) | (MDR_PORTB->RXTX & ~SDATA));
	set_dir(SDATA, OUT);
	set_lvl(nTFS, LOW);
	MDR_TIMER3->CNTRL |= TIMER_CNTRL_CNT_EN;
	while(1) {
		if(MDR_TIMER3->STATUS & TIMER_STATUS_CNT_ARR) { /* NEGEDGE REF */
			ADC_TX_BUF = ADC_TX_BUF << 1;
			MDR_PORTB->RXTX = (((ADC_TX_BUF >> 21) & SDATA) | (MDR_PORTB->RXTX & ~SDATA));
			MDR_TIMER3->STATUS = 0;
			if(++ticks == WR_TICKS_COUNT) {
				MDR_TIMER3->CNTRL &= ~TIMER_CNTRL_CNT_EN;
				ticks = 0;
				break;
			}
		}
	}
	for(uint8_t i = 0; i < 100; i ++) { /* 10.51 мкс, требуется >= 4 тактов SCLK = 8 мкс */
		__asm { NOP }
	}
	set_lvl(nTFS, HIGH);
	set_dir(SDATA, IN);
	MDR_TIMER3->CNT = 0;
}

 
uint32_t adc_rd(void) {
	ADC_RX_BUF = 0;
	set_dir(SDATA, IN);
	set_lvl(nRFS, LOW);
	for(uint8_t i = 0; i < 100; i ++) { /* 10.51 мкс, требуется >= 4 тактов SCLK = 8 мкс */
		__asm { NOP }
	}
	MDR_TIMER3->CNTRL |= TIMER_CNTRL_CNT_EN;
	while(1) {
		if(MDR_TIMER3->STATUS & TIMER_STATUS_CCR_REF_CH3) { /* POSEDGE REF */	 
			ADC_RX_BUF |= (MDR_PORTB->RXTX & SDATA);
			ADC_RX_BUF = ADC_RX_BUF << 1;
			MDR_TIMER3->STATUS = 0;
		}
		if(MDR_TIMER3->STATUS & TIMER_STATUS_CNT_ARR) { /* NEGEDGE REF */
			MDR_TIMER3->STATUS = 0;
			if(++ticks == RD_TICKS_COUNT) {
				MDR_TIMER3->CNTRL &= ~TIMER_CNTRL_CNT_EN;
				ticks = 0;
				break;
			}
		}
	}
	set_lvl(nRFS, HIGH);
	MDR_TIMER3->CNT = 0;
	return ADC_RX_BUF;
}


uint32_t adc_rd_cr(void) {
	uint32_t res = 0;
	set_lvl(A0, LOW);
	res = adc_rd();
	set_lvl(A0, HIGH);
	return res;
}


void adc_wr_cr(uint32_t val) {
	set_lvl(A0, LOW);
	adc_wr(val);
	set_lvl(A0, HIGH);
}


uint32_t adc_rd_dr(void) {
	return adc_rd();
}


void ADC_CTRL_REG_StructInit(ADC_CTRL_REG_TypeDef *cr) {
	uint32_t tmpreg;
	tmpreg |= cr->MD;
	tmpreg |= cr->G;
	tmpreg |= cr->CH;
	tmpreg |= cr->WL;
	tmpreg |= cr->IO;
	tmpreg |= cr->BO;
	tmpreg |= cr->BU;
	tmpreg |= cr->FS;
	adc_wr_cr(tmpreg);
}


/* 50 Гц FS = 382, шум 2 мкВ*/
void adc_sys_clb(void) {
	uint32_t cr = adc_rd_cr();
	cr &= ~MD_Msk; 									/* Очистить поле MD */
	cr |= MD_SYS_CLB_1;							/* Системная калибровка нуля шкалы */
	adc_wr_cr(cr);
	while(MDR_PORTB->RXTX & nDRDY); /* Пока #DRDY = 1 */
	cr = adc_rd_cr();
	/* Очищать поле MD не нужно, т.к. это происходит после калибровки нуля шкалы */ 
	cr |= MD_SYS_CLB_2;							/* Системная калибровка полной шкалы */
	adc_wr_cr(cr);
	while(MDR_PORTB->RXTX & nDRDY); /* Пока #DRDY = 1 */ 
}


uint32_t adc_get_ch1(void) {
	uint32_t adc_cr = adc_rd_cr();
	adc_cr &= ~CH_Msk;
	adc_wr_cr(adc_cr);
	while(MDR_PORTB->RXTX & nDRDY); /* Пока #DRDY = 1 */ 
	return adc_rd_dr();
}


uint32_t adc_get_ch2(void) {
	uint32_t adc_cr = adc_rd_cr();
	adc_cr |= CH_Msk;
	adc_wr_cr(adc_cr);
	while(MDR_PORTB->RXTX & nDRDY); /* Пока #DRDY = 1 */ 
	return adc_rd_dr();
}


/* Интерфейс */
void adc_init(void) {
	port_b_init();
	tim_3_ch_3_init();
	
	ADC_CTRL_REG_TypeDef *cr;
	cr->MD = MD_NORM;
	cr->G  = G_x2;
	cr->CH = CH_1;
	cr->WL = WL_24;
	cr->IO = IO_OFF;
	cr->BO = BO_OFF;
	cr->BU = BU_UPLR;
	cr->FS = (uint32_t)382;
	
	set_lvl(A0, HIGH);
	set_lvl(nTFS, HIGH);
	set_lvl(nRFS, HIGH);
	
	ADC_CTRL_REG_StructInit(cr);
	//adc_sys_clb();
	/* Калибровка 2-го канала */
	//cr->CH = CH_2;
	//ADC_CTRL_REG_StructInit(cr);
	//adc_sys_clb();
}


uint32_t adc_t1_24(void) {
	return adc_get_ch1();
}


uint32_t adc_t2_24(void) {
	return adc_get_ch2();
}
