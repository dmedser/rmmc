#include "stdint.h"
#define MASS_STORAGE_DEVICE_ADDRESS			0x0009

typedef	struct {
   uint8_t 	bmRequestType;
   uint8_t 	bRequest;
   uint16_t wValue;
   uint16_t wIndex;
   uint16_t wLength;
} setup_pck;

extern const setup_pck 	SETUP_PCK_SET_ADDR;
extern const setup_pck 	SETUP_PCK_GET_DESC;
extern const setup_pck 	SETUP_PCK_GET_DESC_CFG;
extern setup_pck 				SETUP_PCK_SET_CFG;
extern setup_pck 				SETUP_PCK_SET_ITF;

extern const setup_pck BULK_ONLY_MASS_STORAGE_RESET;

/* Setup Packet / bmRequestType / Data Phase Transfer Direction */
typedef enum {
  TSF_DIR_HST_TO_DEV  = 0x00,
  TSF_DIR_DEV_TO_HST  = 0x80
} tsf_dir_t;

/* Setup Packet / bmRequestType / Type */
typedef enum {
  REQ_TYPE_STANDARD  = 0x00,
  REQ_TYPE_CLASS     = 0x20,
  REQ_TYPE_VENDOR    = 0x40
} req_type_t;

/* Setup Packet / bmRequestType / Recipient */
typedef enum {
  RECIPIENT_DEV    = 0x0,
  RECIPIENT_ITF		 = 0x1,
  RECIPIENT_EP  	 = 0x2,
  RECIPIENT_OTHER  = 0x3
} recipient_t;

#define RCNT_Msk                		0x1F
#define TSF_DIR_Msk 								0x80
#define REQ_TYPE_Msk 								0x60


#define SCSI_TRANSPARENT_COMMAND_SET_INTERFACE_SUBCLASS_CODE					0x06
#define DEV_DESC_LEN																									18
#define CFG_DESC_LEN																									9
#define ITF_DESC_LEN																									9	
#define EP_DESC_LEN																										7
#define MASS_STORAGE_DEVICE_EXPECTED_ENDPOINTS_NUMBER									2
#define EP_DIR_Pos																										7
#define EP_DIR_OUT																										0x00
#define EP_DIR_IN																				  						0x80
#define EP_NUM_Msk																										0x0F

typedef struct {
	uint8_t 	dev_bLength;
	uint8_t 	dev_bDescriptorType;
	uint8_t 	dev_bcdUSB_l;
	uint8_t 	dev_bcdUSB_h;
	uint8_t  	dev_bDeviceClass;
	uint8_t  	dev_bDeviceSubClass;
	uint8_t  	dev_bDeviceProtocol;
	uint8_t  	dev_bMaxPacketSize;
	uint8_t 	dev_idVendor_l;
	uint8_t 	dev_idVendor_h;
	uint8_t 	dev_idProduct_l;
	uint8_t 	dev_idProduct_h;
	uint8_t 	dev_bcdDevice_l;
	uint8_t 	dev_bcdDevice_h;
	uint8_t  	dev_iManufacturer;
	uint8_t  	dev_iProduct;
	uint8_t  	dev_iSerialNumber;
	uint8_t  	dev_bNumConfigurations;
	
	uint8_t  	cfg_bLength;
	uint8_t  	cfg_bDescriptorType;
	uint8_t 	cfg_wTotalLengt_l;
	uint8_t 	cfg_wTotalLengt_h;
	uint8_t  	cfg_bNumInterfaces;
	uint8_t  	cfg_bConfigurationValue; 
	uint8_t  	cfg_iConfiguration;
	uint8_t  	cfg_bmAttributes;
	uint8_t  	cfg_bMaxPower;
	
	uint8_t  	itf_bLength;
	uint8_t  	itf_bDescriptorType;
	uint8_t  	itf_bInterfaceNumber;
	uint8_t  	itf_bAlternateSetting;
	uint8_t  	itf_bNumEndpoints;
	uint8_t  	itf_bInterfaceClass;
	uint8_t  	itf_bInterfaceSubClass;
	uint8_t  	itf_bInterfaceProtocol;
	uint8_t  	itf_iInterface;
	
	uint8_t   ep_n1_bLength;
	uint8_t   ep_n1_bDescriptorType;
	uint8_t   ep_n1_bEndpointAddress;
	uint8_t   ep_n1_bmAttributes;
	uint8_t   ep_n1_wMaxPacketSize_l;
	uint8_t   ep_n1_wMaxPacketSize_h;
	uint8_t   ep_n1_bInterval;
	
	uint8_t   ep_n2_bLength;
	uint8_t   ep_n2_bDescriptorType;
	uint8_t   ep_n2_bEndpointAddress;
	uint8_t   ep_n2_bmAttributes;
	uint8_t   ep_n2_wMaxPacketSize_l;
	uint8_t   ep_n2_wMaxPacketSize_h;
	uint8_t   ep_n2_bInterval;
} desc_t;


typedef enum {
	TOKEN_TYPE_SETUP = 0x00,
	TOKEN_TYPE_IN,
	TOKEN_TYPE_OUT_DATA0,
	TOKEN_TYPE_OUT_DATA1
} token_type_t;

/* Setup Packet / bRequest */
typedef enum {
  STD_REQ_GET_STATUS = 0,
  STD_REQ_CLEAR_FEATURE,
  RESERVED0,
  STD_REQ_SET_FEATURE,
  RESERVED1,
  STD_REQ_SET_ADDR,
  STD_REQ_GET_DESC,
  STD_REQ_SET_DESC,
  STD_REQ_GET_CFG,
  STD_REQ_SET_CFG,
  STD_REQ_GET_ITF,
  STD_REQ_SET_ITF
} std_reqs;


typedef enum {
  STD_DESC_DEV = 0x0001,
  STD_DESC_CFG = 0x0002,
  STD_DESC_STR = 0x0003,
  STD_DESC_ITF = 0x0004,
  STD_DESC_EP  = 0x0005
} std_descs;


#define USB_HOST_RX_FIFO_STATE	MDR_USB->HRXS
#define USB_HOST_EVENTS					MDR_USB->HIS
#define USB_BUS_STATE						MDR_USB->HRXCS

#define DISCONNECT 							0
#define LOW_SPEED_CONNECT 			1
#define FULL_SPEED_CONNECT			2


typedef enum {
	DEV_CONN_ST_DETACHED,  
	DEV_CONN_ST_ATTACHED
} dev_conn_st_t;


typedef enum {
	HOST_ST_WAIT_CONN, 
	HOST_ST_CONN,
	HOST_ST_WAIT_DESC,
	HOST_ST_SET_ADDR,
	HOST_ST_WAIT_FULL_DESC,
	HOST_ST_WAIT_FULL_DESC_CFG,	
	HOST_ST_SET_CFG,
	HOST_ST_SET_ITF,
	HOST_ST_BOT_RST,
	HOST_ST_CMPL
} host_st_t;


typedef enum {
	TRANS_ANS_NO,
	TRANS_ANS_ACK,
	TRANS_ANS_NAK,
	TRANS_ANS_STALL,
	TRANS_ANS_CRC_ERROR,
	TRANS_ANS_TIMEOUT,
	TRANS_ANS_FIFO_OVERFLOW,
	TRANS_ANS_STUFF_ERROR,
	TRANS_ANS_DATA0,
	TRANS_ANS_DATA1
} trans_ans_t;


void pck_tsf_setup(
	uint8_t 			dev_addr, 
	uint8_t 			ep_addr, 
	token_type_t 	token);



extern dev_conn_st_t 			dev_conn_st;
extern host_st_t 					host_st;
extern trans_ans_t 				ans;
extern uint8_t 						ep_out;
extern uint8_t 						ep_in;
extern token_type_t 			token;
extern uint8_t						dev_addr;


void usb_host_init(void);
void host_process(void);
void bus_rst(void);
void pck_tsf_setup(uint8_t dev_addr, uint8_t ep_addr, token_type_t 	token); 
void enumeration(void);

void rd_f_into_ufd(void);
