#include <stdint.h>
#include "MDR32Fx.h"                    // Device header

/* Инициализация CPU */
void cpu_init(void);

/* Инициализация системного таймера без выбора источника тактирования */
void sys_tim_init(void);

/* Выбор источника тактирования для системного таймера */
void sys_tim_LSI(void);
void sys_tim_HCLK(void);

/* Задержка в тактах на основе системного таймера */
void delay_ticks(uint32_t ticks);
