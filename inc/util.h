#include <stdint.h>
#include "MDR32Fx.h"                    // Device header

/* ������������� CPU */
void cpu_init(void);

/* ������������� ���������� ������� ��� ������ ��������� ������������ */
void sys_tim_init(void);

/* ����� ��������� ������������ ��� ���������� ������� */
void sys_tim_LSI(void);
void sys_tim_HCLK(void);

/* �������� � ������ �� ������ ���������� ������� */
void delay_ticks(uint32_t ticks);
