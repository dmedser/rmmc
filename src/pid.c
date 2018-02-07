#include "pid.h"
#include "flash.h"
#include "MDR32Fx.h"                    // Device header
#include "MDR32F9Qx_timer.h"            // Milandr::Drivers:TIMER
#include "MDR32F9Qx_port.h"             // Milandr::Drivers:PORT
#include "adc.h"

/* Коэффициенты усиления компонент ПИД */
float Kp;
float Ki;
float Kd;

/* Задаваемая температура образца */
uint32_t target_t = 0;

/* Текущая температура образца */
uint32_t curr_t = 0;
/* Температура образца в предыдущем измерении */
uint32_t prev_t = 0;

/* Разница между требуемой и текущей температурой */
uint32_t err = 0;


/* Максимальное/минимальное значение интегральной компоненты */
float I_MAX;
float I_MIN;

/* Статус ПИД регулятора (включен/выключен) */
FunctionalState PID_st = DISABLE;

/* 2-й таймер 3-й канал: ШИМ 10 КГц, настраиваемый коэффициент заполнения */
void tim_2_init(void) {	
	/* Разрешение тактирования TIM2 */
	MDR_RST_CLK->PER_CLOCK |= RST_CLK_PER_CLOCK_PCLK_EN_TIMER2;
	
	TIMER_DeInit(MDR_TIMER2);
	
	/* Установка TIM_CLOCK. Тактовая частота TIM2 = HCLK = 72 МГц, разрешение тактирования */
	TIMER_BRGInit(MDR_TIMER2, TIMER_HCLKdiv1); 
	
	/* Настройка счетчика */
	TIMER_CntInitTypeDef t2_cnt;
	t2_cnt.TIMER_Prescaler 				= 0;	
	t2_cnt.TIMER_CounterMode 			= TIMER_CntMode_ClkFixedDir;
	t2_cnt.TIMER_CounterDirection = TIMER_CntDir_Up;
	TIMER_CntInit(MDR_TIMER2, &t2_cnt);
	
	/* Основание счета, на частоте 72 МГц переполнение через 100 мкс (10 КГц) */
  MDR_TIMER2->ARR = 7200 - 1; 
	
	/* Настройка 3-го канала на ШИМ */
	TIMER_ChnInitTypeDef t2_ch3;
	t2_ch3.TIMER_CH_Number     		 = TIMER_CHANNEL3;
	t2_ch3.TIMER_CH_Mode 	     		 = TIMER_CH_MODE_PWM;
	t2_ch3.TIMER_CH_Prescaler  		 = TIMER_CH_Prescaler_None;
	t2_ch3.TIMER_CH_CCR_UpdateMode = TIMER_CH_CCR_Update_On_CNT_eq_0; /* Обновление CRR при CNT == 0 */
	t2_ch3.TIMER_CH_CCR1_Ena   		 = ENABLE;
	TIMER_ChnInit(MDR_TIMER2, &t2_ch3);
	
	/* Настройка выходного ШИМ сигнала 3-го канала */
	TIMER_ChnOutInitTypeDef t2_ch3_pwm;
	t2_ch3_pwm.TIMER_CH_Number 			  = TIMER_CHANNEL3;
	t2_ch3_pwm.TIMER_CH_DirOut_Source = TIMER_CH_OutSrc_REF;
	t2_ch3_pwm.TIMER_CH_DirOut_Mode   = TIMER_CH_OutMode_Output;
	
	/* Коэффициент заполнения 0.5 - 50 мкс */
	MDR_TIMER2->CCR3  = 3600 - 1;
	MDR_TIMER2->CCR31 = 7200 - 1;
	
	TIMER_ChnOutInit(MDR_TIMER2, &t2_ch3_pwm);
	NVIC_EnableIRQ(TIMER2_IRQn);
}


void tim_1_init(void) {
	/* Таймер 1 Гц: время импульса - ШИМ сигнал ПИД-регулятора */
	
	/* Разрешение тактирования TIM1 */
	MDR_RST_CLK->PER_CLOCK |= RST_CLK_PER_CLOCK_PCLK_EN_TIMER1;
	
	TIMER_DeInit(MDR_TIMER1);
	
	/* Установка TIM_CLOCK. Тактовая частота TIM2 = HCLK = 72 МГц, предделитель 128 */
	TIMER_BRGInit(MDR_TIMER1, TIMER_HCLKdiv128); 
	
	/* Настройка счетчика */
	TIMER_CntInitTypeDef t1_cnt;
	t1_cnt.TIMER_Prescaler 				= 10 - 1;	/* Частота с предделителем = 72 МГц / 128 / 10 = 56250 Гц*/	
	t1_cnt.TIMER_CounterMode 			= TIMER_CntMode_ClkFixedDir;
	t1_cnt.TIMER_CounterDirection = TIMER_CntDir_Up;
	TIMER_CntInit(MDR_TIMER1, &t1_cnt);
	
	/* Основание счета, на частоте 1 Гц переполнение через 500 мс (время импульса) */
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


/* По ТЗ шаг коэффициента заполнения управляющего ШИМ сигнала – не более 1/32
 * Пусть возвращаемое значение ПИД регулятора - натуральное число от 0 до 32
 * и оно будет равно коэффициенту заполнения 
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


/* Включение ПИД регулятора каждую секунду на 500 мс.  
 * Запись текущей температуры в память раз в секунду
 */
void Timer1_IRQHandler(void) {
	/* Если ПИД регулятор выключен */
	if(PID_st == DISABLE) {
		set_pe2_as_pid_ctrl();
		PID_st = ENABLE;
		
		/* Текущая и требуемая к установке температура измеряются в единицах 0.02 гЦ */
		curr_t = adc_t1_24()/3355; 
		err 	 = target_t - curr_t;
		
		/* При допущении, что ПИД регулятор выдает значения от 0 до 32 
		 * и что значение регистра сравнения мб от 0 до 7200 (основание счета таймера)
		 */
		MDR_TIMER2->CCR3 = ((uint16_t)PID(err, curr_t) * 225) - 1;
		
		/* Запись текущей температуры образца и нагревателя-охладителя в память */
		uint32_t t_sample = adc_t2_24();
		f_wr_t(t_sample, w_addr, MAIN);
		w_addr += sizeof(uint32_t);
		f_wr_t(curr_t, w_addr, MAIN);
		w_addr += sizeof(uint32_t);
	}
	/* Если ПИД регулятор включен */
	else {
		set_pe2_0();
		PID_st = DISABLE;
	}
	
	/* Переключение ПИД (вкл/выкл) */
	PID_cmd(PID_st);
}
