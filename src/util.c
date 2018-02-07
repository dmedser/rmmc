#include "MDR32Fx.h"                    // Device header
#include "MDR32F9Qx_rst_clk.h"          // Milandr::Drivers:RST_CLK
#include <stdbool.h>

/* Флаг истечения времени задержки */
bool delay_cmpl = false;

void cpu_init(void) {
	/* Включение HSE */
	/* Режим внешнего генератора */
	MDR_RST_CLK->HS_CONTROL &= ~(1 << RST_CLK_HS_CONTROL_HSE_BYP_Pos); 
	/* Включить внешний HSE */
	MDR_RST_CLK->HS_CONTROL |= RST_CLK_HS_CONTROL_HSE_ON;
	/* Пока HSE не готов */
	while((MDR_RST_CLK->CLOCK_STATUS & RST_CLK_CLOCK_STATUS_HSE_RDY) == 0);

	/* Настройка PLLCPU */
	/* PLLCPUi = CPU_C1 = HSE = 16 МГц */
	MDR_RST_CLK->CPU_CLOCK |= (RST_CLK_CPU_CLOCK_CPU_C1_SEL_HSE << RST_CLK_CPU_CLOCK_CPU_C1_SEL_Pos);
	/* Коэффициент умножения для PLLCPU: PLLCPUo = PLLCPUi x (PLLCPUMUL + 1) */
	/* CPU CLK = 16 МГц(HSE) * 5 = 80 МГц */
	MDR_RST_CLK->PLL_CONTROL |= (4 << RST_CLK_PLL_CONTROL_PLL_CPU_MUL_Pos);
	/* Включение PLLCPU */
	MDR_RST_CLK->PLL_CONTROL |= RST_CLK_PLL_CONTROL_PLL_CPU_ON;
	/* Пока PLLCPU не готов */
	while((MDR_RST_CLK->CLOCK_STATUS & RST_CLK_CLOCK_STATUS_PLL_CPU_RDY) == 0);
		
	/* CPU CLK = HSE->PLL 16*5 = 80 MHz */
	/* HCLK = CPU_C3 */
	MDR_RST_CLK->CPU_CLOCK |= (RST_CLK_CPU_CLOCK_HCLK_SEL_CPU_C3 << RST_CLK_CPU_CLOCK_HCLK_SEL_Pos);
	/* CPU_C3 = CPU_C2 */
	MDR_RST_CLK->CPU_CLOCK |= (RST_CLK_CPU_CLOCK_CPU_C3_SEL_CPU_C2 << RST_CLK_CPU_CLOCK_CPU_C3_SEL_Pos);
	/* CPU_C2 = PLLCPUo */
	MDR_RST_CLK->CPU_CLOCK |= (RST_CLK_CPU_CLOCK_CPU_C2_SEL_PLL_CPU << RST_CLK_CPU_CLOCK_CPU_C2_SEL_Pos);

}


void sys_tim_init(void) {
  NVIC_SetPriority (SysTick_IRQn, (1UL << __NVIC_PRIO_BITS) - 1UL); /* set Priority for Systick Interrupt */
  SysTick->VAL  = 0UL;                                              /* Load the SysTick Counter Value */
	SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
}


void sys_tim_LSI(void) {
	sys_tim_init();
  SysTick->CTRL &= ~(1 << SysTick_CTRL_CLKSOURCE_Pos); /* LSI */
}


void sys_tim_HCLK(void) {
	sys_tim_init();
  SysTick->CTRL |= (1 << SysTick_CTRL_CLKSOURCE_Pos); /* HCLK */
}


void delay_ticks(uint32_t ticks) {
	//SysTick->LOAD  = (uint32_t)(ticks - 1UL);        /* set reload register */
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
	while(!delay_cmpl);
	delay_cmpl = false;
}


void rst_sys_tim(void) {
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
//	NVIC_ClearPendingIRQ(SysTick_IRQn);
}


void SysTick_Handler(void) {
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
	delay_cmpl = true;
}
