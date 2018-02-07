/*-----------------------------------------------------------------------/
/  Low level disk interface modlue include file   (C)ChaN, 2014          /
/-----------------------------------------------------------------------*/

#ifndef _DISKIO_DEFINED
#define _DISKIO_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

//#define _USE_WRITE	1	/* 1: Enable disk_write function */
#define _USE_IOCTL	1	/* 1: Enable disk_ioctl fucntion */

#include "integer.h"
#include "ff.h"
#include <stdint.h>

#ifndef NULL
  #define	NULL	0
#endif

//--------------------------------------------------------------
// Medien-Typen (in dieser LIB MMC und USB)
//--------------------------------------------------------------
typedef enum {
  MMC_0 = 0,      // in dieser LIB wird "MMC"
  USB_1 = 1       // und "USB" unterstuetzt
}MEDIA_t;

//--------------------------------------------------------------
// Fehlercodes
//--------------------------------------------------------------
typedef enum {
  FATFS_OK =0,
  FATFS_NO_MEDIA,
  FATFS_MOUNT_ERR,
  FATFS_GETFREE_ERR,
  FATFS_UNLINK_ERR,
  FATFS_OPEN_ERR,
  FATFS_CLOSE_ERR,
  FATFS_PUTS_ERR,
  FATFS_SEEK_ERR,
  FATFS_RD_STRING_ERR,
  FATFS_RD_BLOCK_ERR,
  FATFS_WR_BLOCK_ERR,
  FATFS_EOF,
  FATFS_DISK_FULL
}FATFS_t;

//--------------------------------------------------------------
// Modes f𲠏penFile
//--------------------------------------------------------------
typedef enum {
  F_RD =0,    // zum lesen oeffnen (nur falls File existiert)
  F_WR,       // zum schreiben oeffnen (nur falls File existiert) und Daten anh寧en
  F_WR_NEW,   // zum schreiben oeffnen (und event. neu anlegen) und Daten anh寧en
  F_WR_CLEAR  // zum schreiben oeffnen (alte Daten loeschen)
}FMODE_t;

//--------------------------------------------------------------
// Globale Funktionen
//--------------------------------------------------------------
FRESULT Fatfs_Mount(MEDIA_t dev);
FATFS_t Fatfs_UnMount(MEDIA_t dev);
	
//=======================================================================================
// ﰨ񠭨塱򰳪򳰻 㱥񻑆AT
typedef struct
{
	DWORD 	sec: 	5;	// ⩲󠰭4, 1 椮 = 2 񥪍
	DWORD	min:	6;	// ⩲󠵭10
	DWORD	hour:	5;	// ⩲󠱱-15
	DWORD	day:	5;	// ⩲󠱶-20
	DWORD	month:	4;	// ⩲󠲱-24
	DWORD	year:	7;	// ⩲󠲵-31
} fat_time_t;
DWORD get_fattime(void);
//=======================================================================================

/* Status of Disk Functions */
typedef BYTE	DSTATUS;

/* Results of Disk Functions */
typedef enum {
	RES_OK = 0,		/* 0: Successful */
	RES_ERROR,		/* 1: R/W Error */
	RES_WRPRT,		/* 2: Write Protected */
	RES_NOTRDY,		/* 3: Not Ready */
	RES_PARERR		/* 4: Invalid Parameter */
} DRESULT;

/*---------------------------------------*/
/* Prototypes for disk control functions */

DSTATUS disk_initialize (BYTE pdrv);
DSTATUS disk_status (BYTE pdrv);
DRESULT disk_read (BYTE pdrv, BYTE* buff, DWORD sector, UINT count);
DRESULT disk_write (BYTE pdrv, const BYTE* buff, DWORD sector, UINT count);
DRESULT disk_ioctl (BYTE pdrv, BYTE cmd, void* buff);

/* Disk Status Bits (DSTATUS) */
#define STA_NOINIT		0x01	/* Drive not initialized */
#define STA_NODISK		0x02	/* No medium in the drive */
#define STA_PROTECT		0x04	/* Write protected */

/* Command code for disk_ioctrl fucntion */

/* Generic command (used by FatFs) */
#define CTRL_SYNC			0	/* Flush disk cache (for write functions) */
#define GET_SECTOR_COUNT	1	/* Get media size (for only f_mkfs()) */
#define GET_SECTOR_SIZE		2	/* Get sector size (for multiple sector size (_MAX_SS >= 1024)) */
#define GET_BLOCK_SIZE		3	/* Get erase block size (for only f_mkfs()) */
#define CTRL_ERASE_SECTOR	4	/* Force erased a block of sectors (for only _USE_ERASE) */

/* Generic command (not used by FatFs) */
#define CTRL_POWER			5	/* Get/Set power status */
#define CTRL_LOCK			6	/* Lock/Unlock media removal */
#define CTRL_EJECT			7	/* Eject media */
#define CTRL_FORMAT			8	/* Create physical format on the media */

/* MMC/SDC specific ioctl command */
#define MMC_GET_TYPE		10	/* Get card type */
#define MMC_GET_CSD			11	/* Get CSD */
#define MMC_GET_CID			12	/* Get CID */
#define MMC_GET_OCR			13	/* Get OCR */
#define MMC_GET_SDSTAT		14	/* Get SD status */

/* ATA/CF specific ioctl command */
#define ATA_GET_REV			20	/* Get F/W revision */
#define ATA_GET_MODEL		21	/* Get model name */
#define ATA_GET_SN			22	/* Get serial number */

/* MMC card type flags (MMC_GET_TYPE) */
#define CT_MMC		0x01		/* MMC ver 3 */
#define CT_SD1		0x02		/* SD ver 1 */
#define CT_SD2		0x04		/* SD ver 2 */
#define CT_SDC		(CT_SD1|CT_SD2)	/* SD */
#define CT_BLOCK	0x08		/* Block addressing */

#ifdef __cplusplus
}
#endif

#endif
