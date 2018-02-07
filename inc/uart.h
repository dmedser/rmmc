#include <stdint.h>
#include <stdbool.h>

/* ���� ��������� ������� ������� �� UART */
extern bool 		ext_cmd_received;

/* ��� �������� � �������� ������� ������� */
extern uint8_t  ext_cmd_opcode;
extern uint16_t ext_cmd_param;

/* ���� �������� ������� ������� */
#define STATE										(uint8_t)0x0A
#define SET_TMP									(uint8_t)0x0B
#define RD_INTO									(uint8_t)0x0C

/* ��������� ������� ������� RD_INTO */
#define UART										(uint16_t)0x0000
#define USB_FLASH_DRIVE					(uint16_t)0x0001

/* �������������� ��������� ����� ������ ������ �� UART */
void u_wr_byte(uint8_t data);
void u_wr_buf(uint8_t *buf, uint8_t len);

/* ��������� */
/* ������������� UART */
void uart_init(void);
/* ������ ��������������� �������� ����������� */
void u_wr_tH(uint32_t tH);
/* ������ ���� ������ �� ���� ������ � UART */
void rd_f_into_u(void);
