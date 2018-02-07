#include <stdint.h>
#include "MDR32Fx.h"  

/* ���� ����������/���������� ����������� ��� �������� 
 * ��� ��������� ������ �� ������ ������������ ����������� 
 * ����������� � ������������ ��������.
 * �.�. ��� ���������� �������� ������ ���������� ������ ������������
 * ������ ������� ������� � ���������� ��� �������
 */

/* ���������� ����������� ������� */
extern uint32_t target_t;

/* �������������� ������ */
/* ������ ��������� ��� ������� ���������� */
void 		tim_2_init(void);

/* ������ ��������� ������� ������� ������������ ����������� */
void 		tim_1_init(void);

/* ������� ������ ������ ������ ��� ���������� */
void 		set_pe2_as_pid_ctrl(void);
void	 	set_pe2_0(void);

/* ��� ��������� ������ ��������, ���������������� ������������ ���������� ��� ������� */
uint8_t PID_ctrl(uint16_t err, uint16_t curr_t);

/* ����������/������ ������ ��� ���������� */
void 		PID_cmd(FunctionalState st);


/* ��������� */
/* ������������� ������� ��������� �������� ������� */
void carrier_init(void);

/* ���������/���������� �������� ������� ���������� �������� ������ 
 * �� �������������� ������ ������� �������������� �������� ����� ��� 
 */
void carrier_cmd(FunctionalState st);
