#include "stdint.h"
/* Control Transfer Stages */
typedef enum {
	CTRL_TSF_STAGE_SETUP_BEGIN,  
	CTRL_TSF_STAGE_SETUP_END,
	CTRL_TSF_STAGE_DATA_BEGIN,
	CTRL_TSF_STAGE_DATA_END,
	CTRL_TSF_STAGE_STATUS_BEGIN,
	CTRL_TSF_STAGE_STATUS_END,
	CTRL_TSF_COMPLETE,
	CTRL_TSF_ERROR
} ctrl_tsf_stage_t;

/* Control Transfer Types */
typedef enum {
	CTRL_TSF_TYPE_WRITE,
	CTRL_TSF_TYPE_READ
} ctrl_tsf_type_t;

/* Control Transfer Setup */
void ctrl_tsf_setup(uint8_t 					dev_addr,						/* ����� ���������� */
										uint8_t 					*req_ptr,						/* ��������� �� ������ */
										uint8_t 					*data_ptr,					/* ��������� �� ������ */
										uint32_t					stage_bytes_num,		/* ���������� ���� � ����� ������ ������ */
										uint32_t 					stages_num,					/* ���������� ������ ������ */
										ctrl_tsf_type_t		ctrl_tsf_type);	 		/* ��� �������� */

/* Control Transfer */
ctrl_tsf_stage_t ctrl_tsf(void);
ctrl_tsf_stage_t reset_recovery(void);
