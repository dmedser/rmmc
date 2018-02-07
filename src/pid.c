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

/* 2-� ������ 3-� �����: ��� 10 ���, ������������� ����������� ���������� */
void tim_2_init(void) {	
	/* ���������� ������������ TIM2 */
	MDR_RST_CLK->PER_CLOCK |= RST_CLK_PER_CLOCK_PCLK_EN_TIMER2;
	
	TIMER_DeInit(MDR_TIMER2);
	
	/* ��������� TIM_CLOCK. �������� ������� TIM2 = HCLK = 72 ���, ���������� ������������ */
	TIMER_BRGInit(MDR_TIMER2, TIMER_HCLKdiv1); 
	
	/* ��������� �������� */
	TIMER_CntInitTypeDef t2_cnt;
	t2_cnt.TIMER_Prescaler 				= 0;	
	t2_cnt.TIMER_CounterMode 			= TIMER_CntMode_ClkFixedDir;
	t2_cnt.TIMER_CounterDirection = TIMER_CntDir_Up;
	TIMER_CntInit(MDR_TIMER2, &t2_cnt);
	
	/* ��������� �����, �� ������� 72 ��� ������������ ����� 100 ��� (10 ���) */
  MDR_TIMER2->ARR = 7200 - 1; 
	
	/* ��������� 3-�� ������ �� ��� */
	TIMER_ChnInitTypeDef t2_ch3;
	t2_ch3.TIMER_CH_Number     		 = TIMER_CHANNEL3;
	t2_ch3.TIMER_CH_Mode 	     		 = TIMER_CH_MODE_PWM;
	t2_ch3.TIMER_CH_Prescaler  		 = TIMER_CH_Prescaler_None;
	t2_ch3.TIMER_CH_CCR_UpdateMode = TIMER_CH_CCR_Update_On_CNT_eq_0; /* ���������� CRR ��� CNT == 0 */
	t2_ch3.TIMER_CH_CCR1_Ena   		 = ENABLE;
	TIMER_ChnInit(MDR_TIMER2, &t2_ch3);
	
	/* ��������� ��������� ��� ������� 3-�� ������ */
	TIMER_ChnOutInitTypeDef t2_ch3_pwm;
	t2_ch3_pwm.TIMER_CH_Number 			  = TIMER_CHANNEL3;
	t2_ch3_pwm.TIMER_CH_DirOut_Source = TIMER_CH_OutSrc_REF;
	t2_ch3_pwm.TIMER_CH_DirOut_Mode   = TIMER_CH_OutMode_Output;
	
	/* ����������� ���������� 0.5 - 50 ��� */
	MDR_TIMER2->CCR3  = 3600 - 1;
	MDR_TIMER2->CCR31 = 7200 - 1;
	
	TIMER_ChnOutInit(MDR_TIMER2, &t2_ch3_pwm);
	NVIC_EnableIRQ(TIMER2_IRQn);
}


void tim_1_init(void) {
	/* ������ 1 ��: ����� �������� - ��� ������ ���-���������� */
	
	/* ���������� ������������ TIM1 */
	MDR_RST_CLK->PER_CLOCK |= RST_CLK_PER_CLOCK_PCLK_EN_TIMER1;
	
	TIMER_DeInit(MDR_TIMER1);
	
	/* ��������� TIM_CLOCK. �������� ������� TIM2 = HCLK = 72 ���, ������������ 128 */
	TIMER_BRGInit(MDR_TIMER1, TIMER_HCLKdiv128); 
	
	/* ��������� �������� */
	TIMER_CntInitTypeDef t1_cnt;
	t1_cnt.TIMER_Prescaler 				= 10 - 1;	/* ������� � ������������� = 72 ��� / 128 / 10 = 56250 ��*/	
	t1_cnt.TIMER_CounterMode 			= TIMER_CntMode_ClkFixedDir;
	t1_cnt.TIMER_CounterDirection = TIMER_CntDir_Up;
	TIMER_CntInit(MDR_TIMER1, &t1_cnt);
	
	/* ��������� �����, �� ������� 1 �� ������������ ����� 500 �� (����� ��������) */
  MDR_TIMER1->ARR = 28125 - 1;
	NVIC_EnableIRQ(TIMER1_IRQn);
}


void set_pe2_as_pid_ctrl(void) {
	MDR_RST_CLK->PER_CLOCK |= RST_CLK_PER_CLOCK_PCLK_EN_PORTE;
	
	/* PE2 - TIM2 CH3 PWM */
	PORT_InitTypeDef pe2;
	pe2.PORT_Pin 	 = PORT_Pin_2;
	pe2.PORT_OE 	 = PORT_OE_OUT;
	pe2.PORT_MODE  = PORT_MODE_DIGITAL;
	pe2.PORT_FUNC  = PORT_FUNC_ALTER;
	pe2.PORT_SPEED = PORT_SPEED_FAST; 	 	/* should be <= 50 ns */
	PORT_Init(MDR_PORTE, &pe2);
}

void set_pe2_0(void) {
	PORT_InitTypeDef pe2;
	pe2.PORT_Pin 	 = PORT_Pin_2;
	pe2.PORT_OE 	 = PORT_OE_OUT;
	pe2.PORT_MODE  = PORT_MODE_DIGITAL;
	pe2.PORT_FUNC  = PORT_FUNC_PORT;
	pe2.PORT_SPEED = PORT_SPEED_FAST; 	 	/* should be <= 50 ns */
	PORT_Init(MDR_PORTE, &pe2);
	PORT_WriteBit(MDR_PORTE, PORT_Pin_2, Bit_RESET);
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
	TIMER_Cmd(MDR_TIMER2, st);
}


void carrier_init(void) {
	tim_1_init();
}


void carrier_cmd(FunctionalState st) {
	TIMER_Cmd(MDR_TIMER1, st);
}


/* ��������� ��� ���������� ������ ������� �� 500 ��.  
 * ������ ������� ����������� � ������ ��� � �������
 */
void Timer1_IRQHandler(void) {
	/* ���� ��� ��������� �������� */
	if(PID_st == DISABLE) {
		set_pe2_as_pid_ctrl();
		PID_st = ENABLE;
		
		/* ������� � ��������� � ��������� ����������� ���������� � �������� 0.02 �� */
		curr_t = adc_t1_24()/3355; 
		err 	 = target_t - curr_t;
		
		/* ��� ���������, ��� ��� ��������� ������ �������� �� 0 �� 32 
		 * � ��� �������� �������� ��������� �� �� 0 �� 7200 (��������� ����� �������)
		 */
		MDR_TIMER2->CCR3 = ((uint16_t)PID(err, curr_t) * 225) - 1;
		
		/* ������ ������� ����������� ������� � �����������-���������� � ������ */
		uint32_t t_sample = adc_t2_24();
		f_wr_t(t_sample, w_addr, MAIN);
		w_addr += sizeof(uint32_t);
		f_wr_t(curr_t, w_addr, MAIN);
		w_addr += sizeof(uint32_t);
	}
	/* ���� ��� ��������� ������� */
	else {
		set_pe2_0();
		PID_st = DISABLE;
	}
	
	/* ������������ ��� (���/����) */
	PID_cmd(PID_st);
}
