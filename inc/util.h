#include <stdint.h>
#include "MDR32Fx.h"                    // Device header

/* Инициализация CPU */
void cpu_init(void);

/* Инициализация системного таймера без выбора источника тактирования */
void sys_tim_init(void);

/* Задержка в тактах на основе системного таймера */
void delay_ms(uint32_t ms);
