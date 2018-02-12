#include "pid.h"
#include "flash.h"
#include "MDR32Fx.h"                    // Device header
#include "MDR32F9Qx_timer.h"            // Milandr::Drivers:TIMER
#include "MDR32F9Qx_port.h"             // Milandr::Drivers:PORT
#include "adc.h"

/* ������������ �������� ��������� ��� */
float Kp;
float Ki;
float Kd;

/* ���������� ����������� ������� */
uint32_t target_t = 0;

/* ������� ����������� ������� */
uint32_t curr_t = 0;
/* ����������� ������� � ���������� ��������� */
uint32_t prev_t = 0;

/* ������� ����� ��������� � ������� ������������ */
uint32_t err = 0;


/* ������������/����������� �������� ������������ ���������� */
float I_MAX;
float I_MIN;

/* ������ ��� ���������� (�������/��������) */
FunctionalState PID_st = DISABLE;


/* PE2 ����� ��� */
void port_e_init(void) {
	MDR_RST_CLK->PER_CLOCK |= RST_CLK_PER_CLOCK_PCLK_EN_PORTE;
	/* PE2 - TIM2 CH3 PWM */
	PORT_InitTypeDef pe2;
	pe2.PORT_Pin 	 = PORT_Pin_2;
	pe2.PORT_OE 	 = PORT_OE_OUT;
	pe2.PORT_MODE  = PORT_MODE_DIGITAL;
	pe2.PORT_SPEED = PORT_SPEED_FAST; 	 	/* should be <= 50 ns */
	PORT_Init(MDR_PORTE, &pe2);
}


/* 2-� ������ 3-� �����: ��� 10 ���, ������������� ����������� ���������� */
void tim_2_init(void) {	
	TIMER_DeInit(MDR_TIMER2);
	/* ���������� ������������ TIM2 */
	MDR_RST_CLK->PER_CLOCK |= RST_CLK_PER_CLOCK_PCLK_EN_TIMER2;
  /* ��������� ��������� ������������ TIM2 */
	/* HCLK ��� ����������� */
	MDR_RST_CLK->TIM_CLOCK |= (RST_CLK_TIM_CLOCK_TIM_BRG_HCLK << RST_CLK_TIM_CLOCK_TIM2_BRG_Pos);
	/* ���������� ������������ TIM2 */
	MDR_RST_CLK->TIM_CLOCK |= RST_CLK_TIM_CLOCK_TIM2_CLK_EN;
	
	/* ��������� �������� */	
	MDR_TIMER2->CNT = 0;
	/* ��� ����������� */
	MDR_TIMER2->PSG = 0;
	/* ��������� ����� */
  MDR_TIMER2->ARR = 8000 - 1;  					/* 100 ��� */
	
	/* ����������� ����� - ����� �� 0 �� ARR */
	MDR_TIMER2->CNTRL &= ~(1 << TIMER_CNTRL_DIR_Pos); 
	MDR_TIMER2->CNTRL &= ~(0x3 << TIMER_CNTRL_CNT_MODE_Pos);
	
	/* ������ ������� ��������� */
	MDR_TIMER2->CCR3  = 4000 - 1;         /* 50 ���, ����������� ���������� 0.5 */
  /* ������ ������� ��������� = ARR */
  MDR_TIMER2->CCR31 = 8000 - 1;         /* 100 ��� */
	
	/* ��������� ������ */
	/* ��� ����������� */
	MDR_TIMER2->CH3_CNTRL &= ~(0x3 << TIMER_CH_CNTRL_CHPSC_Pos); 
	/* ����� ������ - ��� */
	MDR_TIMER2->CH3_CNTRL &= ~(1 << TIMER_CH_CNTRL_CAP_NPWM_Pos);
	/* ������ ��������� ������� REF - ������������ REF ���� CNT == CCR ��� CNT == CRR1 */
	MDR_TIMER2->CH3_CNTRL |= (TIMER_CH_CNTRL_OCCM_SW_REF_CNT_CCR_OR_CNT_CCR1 << TIMER_CH_CNTRL_OCCM_Pos);
	/* ������ ����� ������ �������� �� ����� */
	MDR_TIMER2->CH3_CNTRL1 |= (TIMER_CH_CNTRL1_SELOE_OUT_EN << TIMER_CH_CNTRL1_SELOE_Pos);
	/* �� ����� �������� REF */
 	MDR_TIMER2->CH3_CNTRL1 |= (TIMER_CH_CNTRL1_SELO_OUT_REF << TIMER_CH_CNTRL1_SELO_Pos);
	/* ���������� CRR1 */
	MDR_TIMER2->CH3_CNTRL2 |= TIMER_CH_CNTRL2_CCR1_EN;

}

/* ������ 1 ��: ����� �������� - ��� ������ ���-���������� */
void tim_1_init(void) {
	TIMER_DeInit(MDR_TIMER1);
	/* ���������� ������������ TIM1 */
	MDR_RST_CLK->PER_CLOCK |= RST_CLK_PER_CLOCK_PCLK_EN_TIMER1;
	/* ��������� ��������� ������������ TIM1 */
	/* HCLK c ������������ �� 128 */
	MDR_RST_CLK->TIM_CLOCK |= (RST_CLK_TIM_CLOCK_TIM_BRG_HCLK_DIV_128 << RST_CLK_TIM_CLOCK_TIM1_BRG_Pos);
	/* ���������� ������������ TIM1 */
	MDR_RST_CLK->TIM_CLOCK |= RST_CLK_TIM_CLOCK_TIM1_CLK_EN;
	
	/* ��������� �������� */	
	MDR_TIMER1->CNT = 0;
	/* ������� � ������������� = 80 ��� / 128 / 10 = 62500 �� */	
	MDR_TIMER1->PSG = 10 - 1;
	/* ��������� ����� �� ������� 62500 �� ������������ ����� 500 �� (����� ��������) */
  MDR_TIMER1->ARR = 31250 - 1; 
	
	/* ����������� ����� - ����� �� 0 �� ARR */
	MDR_TIMER1->CNTRL &= ~(1 << TIMER_CNTRL_DIR_Pos); 
	MDR_TIMER1->CNTRL &= ~(0x3 << TIMER_CNTRL_CNT_MODE_Pos);
	
	/* ���������� �� ������������ */
	MDR_TIMER1->IE |= TIMER_STATUS_CNT_ARR;
	NVIC_EnableIRQ(TIMER1_IRQn);
}


void set_pe2_as_pid_ctrl(void) {
	MDR_PORTE->FUNC |= (PORT_FUNC_MODE_ALT << PORT_FUNC_MODE2_Pos);
}

void set_pe2_0(void) {
	MDR_PORTE->FUNC &= ~PORT_FUNC_MODE2_Msk;
}


/* �� �� ��� ������������ ���������� ������������ ��� ������� � �� ����� 1/32
 * ����� ������������ �������� ��� ���������� - ����������� ����� �� 0 �� 32
 * � ��� ����� ����� ������������ ���������� 
 */
uint8_t PID(uint32_t err, uint32_t curr_t) {
	float P, I, D;
	P = Kp * err;
	I += err;
	if(I > I_MAX) {
		I = I_MAX;
	}
	else if(I < I_MIN){
		I = I_MIN;
	}
	I = Ki * I;
	D = Kd * (curr_t - prev_t);
	prev_t = curr_t;
	return (uint8_t)(P + I + D);
}


void PID_cmd(FunctionalState st) {
	if(st == ENABLE) {
		MDR_TIMER2->CNTRL |= TIMER_CNTRL_CNT_EN;
	}
	else {
		MDR_TIMER2->CNTRL &= ~ TIMER_CNTRL_CNT_EN;
	}
}


void carrier_init(void) {
	tim_1_init();
	tim_2_init();
	port_e_init();
}


void carrier_cmd(FunctionalState st) {
	TIMER_Cmd(MDR_TIMER1, st);
}


/* ��������� ��� ���������� ������ ������� �� 500 ��.  
 * ������ ������� ����������� � ������ ��� � �������
 */
void Timer1_IRQHandler(void) {
	MDR_TIMER1->STATUS = 0;
	/* ���� ��� ��������� �������� */
	if(PID_st == DISABLE) {
		set_pe2_as_pid_ctrl();
		
		PID_st = ENABLE;
////		
////		/* ������� � ��������� � ��������� ����������� ���������� � �������� 0.02 �� */
////		curr_t = adc_t1()/3355; 
////		err 	 = target_t - curr_t;
////		
////		/* ��� ���������, ��� ��� ��������� ������ �������� �� 0 �� 32 
////		 * � ��� �������� �������� ��������� �� �� 0 �� 8000 (��������� ����� �������)
////		 */
////		MDR_TIMER2->CCR3 = ((uint16_t)PID(err, curr_t) * 250) - 1;
////		
////		/* ������ ������� ����������� ������� � �����������-���������� � ������ */
////		uint32_t t_sample = adc_t2();
////		f_wr_t(t_sample, w_addr, MAIN);
////		w_addr += sizeof(uint32_t);
////		f_wr_t(curr_t, w_addr, MAIN);
////		w_addr += sizeof(uint32_t);
	}
	/* ���� ��� ��������� ������� */
	else {
		set_pe2_0();
		PID_st = DISABLE;
	}
	
	/* ������������ ��� (���/����) */
	PID_cmd(PID_st);
}
