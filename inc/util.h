#include <stdint.h>
#include "MDR32Fx.h"                    // Device header

/* ������������� CPU */
void cpu_init(void);

/* ������������� ���������� ������� ��� ������ ��������� ������������ */
void sys_tim_init(void);

/* �������� � ������ �� ������ ���������� ������� */
void delay_ms(uint32_t ms);
