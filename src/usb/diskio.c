/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2016        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "diskio.h"		/* FatFs lower layer API */
//#include "main.h"
#include "bulk_transfer.h"
#include <string.h> /* memset, memcpy */

/* Definitions of physical drive number for each drive */
#define DEV_RAM		0	/* Example: Map Ramdisk to physical drive 0 */
#define DEV_MMC		1	/* Example: Map MMC/SD card to physical drive 1 */
#define DEV_USB		2	/* Example: Map USB MSD to physical drive 2 */

static volatile DSTATUS Stat = STA_NOINIT;	/* Disk status */

extern uint8_t blk_tsf_buf[BULK_TRANSFER_BUFFER_LENGTH];
extern DWORD LBA;
extern short transferlenth;
extern blk_only_csw_pck CSW;

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/
DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS status;
	BOT_stat();
	BOT_init();
    if (CSW.bStatus == 0)
	{
		status &= STA_NOINIT;
		status &= STA_NODISK;
		status &= STA_PROTECT;
	} 
	else 
	{ 
		status = STA_NOINIT;
	}
	return status;
}


/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/
DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS status;
	BOT_request_sense();
	BOT_stat();
	BOT_init();
	BOT_inquiry();
	if (CSW.bStatus == 0)
	{
		status &= STA_NOINIT;
		status &= STA_NODISK;
		status &= STA_PROTECT;
	}
	else
	{		
		status = STA_NOINIT;
	}
	return status;	
}


/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/
DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	DRESULT result;

	LBA = sector;
	transferlenth = 1;

	for(uint32_t i = 0; i < count; i++)
	{	
		BOT_in();			
		LBA++;
		memcpy(buff, blk_tsf_buf, BULK_TRANSFER_BUFFER_LENGTH);
		memset(blk_tsf_buf, 0, BULK_TRANSFER_BUFFER_LENGTH);
	}
	if(CSW.bStatus == 0)
	{
		result = RES_OK;
	}
	else 
	{
		result = RES_NOTRDY;
	}
	return result;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/
DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	DRESULT result;
	LBA = sector;
	transferlenth = 1;

	for (uint32_t i = 0; i < count; i++)
	{
		memcpy(blk_tsf_buf, buff + i*BULK_TRANSFER_BUFFER_LENGTH, BULK_TRANSFER_BUFFER_LENGTH);
		
		BOT_out();
		
		LBA++;
		memset(blk_tsf_buf, 0, BULK_TRANSFER_BUFFER_LENGTH);
	}
	if (CSW.bStatus == 0)
	{
		result = RES_OK;
	} 
	else 
	{
		result = RES_NOTRDY;
	}
	return result;
}


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/
DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res;
//	int result;

	switch (pdrv) {

	case DEV_USB :

		// Process of the command the USB drive

		return res;
	}

	return RES_PARERR;
}

