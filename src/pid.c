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


/* PE2 вывод ШИМ */
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


/* 2-й таймер 3-й канал: ШИМ 10 КГц, настраиваемый коэффициент заполнения */
void tim_2_init(void) {	
	TIMER_DeInit(MDR_TIMER2);
	/* Разрешение тактирования TIM2 */
	MDR_RST_CLK->PER_CLOCK |= RST_CLK_PER_CLOCK_PCLK_EN_TIMER2;
  /* Настройка источника тактирования TIM2 */
	/* HCLK без предделения */
	MDR_RST_CLK->TIM_CLOCK |= (RST_CLK_TIM_CLOCK_TIM_BRG_HCLK << RST_CLK_TIM_CLOCK_TIM2_BRG_Pos);
	/* Разрешение тактирования TIM2 */
	MDR_RST_CLK->TIM_CLOCK |= RST_CLK_TIM_CLOCK_TIM2_CLK_EN;
	
	/* Обнуление счетчика */	
	MDR_TIMER2->CNT = 0;
	/* Без предделения */
	MDR_TIMER2->PSG = 0;
	/* Основание счета */
  MDR_TIMER2->ARR = 8000 - 1;  					/* 100 мкс */
	
	/* Направление счета - вверх от 0 до ARR */
	MDR_TIMER2->CNTRL &= ~(1 << TIMER_CNTRL_DIR_Pos); 
	MDR_TIMER2->CNTRL &= ~(0x3 << TIMER_CNTRL_CNT_MODE_Pos);
	
	/* Первый регистр сравнения */
	MDR_TIMER2->CCR3  = 4000 - 1;         /* 50 мкс, коэффициент заполнения 0.5 */
  /* Второй регистр сравнения = ARR */
  MDR_TIMER2->CCR31 = 8000 - 1;         /* 100 мкс */
	
	/* Настройка канала */
	/* Без предделения */
	MDR_TIMER2->CH3_CNTRL &= ~(0x3 << TIMER_CH_CNTRL_CHPSC_Pos); 
	/* Режим работы - ШИМ */
	MDR_TIMER2->CH3_CNTRL &= ~(1 << TIMER_CH_CNTRL_CAP_NPWM_Pos);
	/* Формат выработки сигнала REF - переключение REF если CNT == CCR или CNT == CRR1 */
	MDR_TIMER2->CH3_CNTRL |= (TIMER_CH_CNTRL_OCCM_SW_REF_CNT_CCR_OR_CNT_CCR1 << TIMER_CH_CNTRL_OCCM_Pos);
	/* Прямой канал всегда работает на выход */
	MDR_TIMER2->CH3_CNTRL1 |= (TIMER_CH_CNTRL1_SELOE_OUT_EN << TIMER_CH_CNTRL1_SELOE_Pos);
	/* На выход подается REF */
 	MDR_TIMER2->CH3_CNTRL1 |= (TIMER_CH_CNTRL1_SELO_OUT_REF << TIMER_CH_CNTRL1_SELO_Pos);
	/* Разрешение CRR1 */
	MDR_TIMER2->CH3_CNTRL2 |= TIMER_CH_CNTRL2_CCR1_EN;

}

/* Таймер 1 Гц: время импульса - ШИМ сигнал ПИД-регулятора */
void tim_1_init(void) {
	TIMER_DeInit(MDR_TIMER1);
	/* Разрешение тактирования TIM1 */
	MDR_RST_CLK->PER_CLOCK |= RST_CLK_PER_CLOCK_PCLK_EN_TIMER1;
	/* Настройка источника тактирования TIM1 */
	/* HCLK c предделением на 128 */
	MDR_RST_CLK->TIM_CLOCK |= (RST_CLK_TIM_CLOCK_TIM_BRG_HCLK_DIV_128 << RST_CLK_TIM_CLOCK_TIM1_BRG_Pos);
	/* Разрешение тактирования TIM1 */
	MDR_RST_CLK->TIM_CLOCK |= RST_CLK_TIM_CLOCK_TIM1_CLK_EN;
	
	/* Обнуление счетчика */	
	MDR_TIMER1->CNT = 0;
	/* Частота с предделителем = 80 МГц / 128 / 10 = 62500 Гц */	
	MDR_TIMER1->PSG = 10 - 1;
	/* Основание счета на частоте 62500 Гц переполнение через 500 мс (время импульса) */
  MDR_TIMER1->ARR = 31250 - 1; 
	
	/* Направление счета - вверх от 0 до ARR */
	MDR_TIMER1->CNTRL &= ~(1 << TIMER_CNTRL_DIR_Pos); 
	MDR_TIMER1->CNTRL &= ~(0x3 << TIMER_CNTRL_CNT_MODE_Pos);
	
	/* Прерывание по переполнению */
	MDR_TIMER1->IE |= TIMER_STATUS_CNT_ARR;
	NVIC_EnableIRQ(TIMER1_IRQn);
}


void set_pe2_as_pid_ctrl(void) {
	MDR_PORTE->FUNC |= (PORT_FUNC_MODE_ALT << PORT_FUNC_MODE2_Pos);
}

void set_pe2_0(void) {
	MDR_PORTE->FUNC &= ~PORT_FUNC_MODE2_Msk;
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


/* Включение ПИД регулятора каждую секунду на 500 мс.  
 * Запись текущей температуры в память раз в секунду
 */
void Timer1_IRQHandler(void) {
	MDR_TIMER1->STATUS = 0;
	/* Если ПИД регулятор выключен */
	if(PID_st == DISABLE) {
		set_pe2_as_pid_ctrl();
		
		PID_st = ENABLE;
////		
////		/* Текущая и требуемая к установке температура измеряются в единицах 0.02 гЦ */
////		curr_t = adc_t1()/3355; 
////		err 	 = target_t - curr_t;
////		
////		/* При допущении, что ПИД регулятор выдает значения от 0 до 32 
////		 * и что значение регистра сравнения мб от 0 до 8000 (основание счета таймера)
////		 */
////		MDR_TIMER2->CCR3 = ((uint16_t)PID(err, curr_t) * 250) - 1;
////		
////		/* Запись текущей температуры образца и нагревателя-охладителя в память */
////		uint32_t t_sample = adc_t2();
////		f_wr_t(t_sample, w_addr, MAIN);
////		w_addr += sizeof(uint32_t);
////		f_wr_t(curr_t, w_addr, MAIN);
////		w_addr += sizeof(uint32_t);
	}
	/* Если ПИД регулятор включен */
	else {
		set_pe2_0();
		PID_st = DISABLE;
	}
	
	/* Переключение ПИД (вкл/выкл) */
	PID_cmd(PID_st);
}
