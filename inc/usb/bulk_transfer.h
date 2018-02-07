#include "stdint.h"
#define BULK_TRANSFER_BUFFER_LENGTH					512

#define SCSI_WRITE10                                0x2A
#define SCSI_READ10                                 0x28
#define SCSI_VERIFY10                               0x2F
#define SCSI_READ_CAPACITY10                        0x25
#define SCSI_READ_FORMAT_CAPACITIES                 0x23
#define SCSI_REQUEST_SENSE                          0x03
#define SCSI_INQUIRY                                0x12
#define SCSI_TEST_UNIT_READY                        0x00
#define SCSI_FORMAT_UNIT                            0x04
#define SCSI_SEND_DIAGNOSTIC                        0x1D
#define SCSI_START_STOP_UNIT                        0x1B
#define SCSI_MODE_SENSE                       		0x5A

#define BOT_CBW_SIGNATURE             				0x43425355
#define BOT_CSW_SIGNATURE             				0x53425355
#define BOT_CBW_PACKET_LENGTH         				31

/*Bulk-only Command block Wrapper */
typedef struct {
  uint32_t 	dSignature;
  uint32_t 	dTag;
  uint32_t 	dDataLength;
  uint8_t  	bmFlags;
  uint8_t  	bLUN;
  uint8_t  	bCBLength;
  uint8_t  	CB[16];
} blk_only_cbw_pck;


/* Bulk-only Command Status Wrapper */
typedef struct {
  uint32_t 	dSignature;
  uint32_t 	dTag;
  uint32_t 	dDataResidue;
  uint8_t  	bStatus;
} blk_only_csw_pck;


typedef struct {
  uint32_t lastLBA;   /*RETURNED LOGICAL BLOCK ADDRESS*/
  uint32_t block_lenght; /*BLOCK LENGTH IN BYTES*/	
} CAPACITY10;



typedef enum {
	BLK_TSF_STAGE_CBW_BEGIN,  
	BLK_TSF_STAGE_CBW_END,
	BLK_TSF_STAGE_DATA_BEGIN,
	BLK_TSF_STAGE_DATA_END,
	BLK_TSF_STAGE_CSW_BEGIN,
	BLK_TSF_STAGE_CSW_END,
	BLK_TSF_COMPLETE,
	BLK_TSF_ERROR
} blk_tsf_stage_t;

typedef enum {
	BLK_TSF_TYPE_IN,  
	BLK_TSF_TYPE_OUT
} blk_tsf_type_t;

blk_tsf_stage_t BOT_out(void);
blk_tsf_stage_t BOT_in(void);
blk_tsf_stage_t BOT_init(void);
blk_tsf_stage_t BOT_stat(void);
blk_tsf_stage_t BOT_request_sense(void);
blk_tsf_stage_t BOT_inquiry(void);

void blk_tsf_setup(uint32_t dev_addr, uint8_t *cbw_ptr, uint8_t *bt_data_ptr, uint8_t *csw_ptr, uint32_t bt_stages_num, uint32_t bt_stage_bytes_num, blk_tsf_type_t bt_type);
blk_tsf_stage_t blk_tsf_t(void);

