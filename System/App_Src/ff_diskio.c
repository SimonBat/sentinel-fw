/**
  ***************************************************************************************************************************************
  * @file     ff_diskio.c
  * @owner    SimonBat
  * @version  v0.0.1
  * @date     2021.09.06
  * @update   2021.09.06
  * @brief    sentinel v1.0
  ***************************************************************************************************************************************
  * @attention
  *
  * (Where to use)
  *
  ***************************************************************************************************************************************
  */

#include <string.h>
#include "ff_gen_drv.h"
#include "n25q512a_qspi.h"

/* Disk status */
#define DISKIO_BLK_NBR	0x4000
#define DISKIO_BLK_SIZ  0x1000
static volatile DSTATUS Stat = STA_NOINIT;

/* Private function prototypes */
DSTATUS USER_initialize(BYTE _pdrv);
DSTATUS USER_status(BYTE _pdrv);
DRESULT USER_read(BYTE _pdrv, BYTE *_buff, DWORD _sector, UINT _count);
#if (1U == _USE_WRITE)
	DRESULT USER_write(BYTE _pdrv, const BYTE *_buff, DWORD _sector, UINT _count);
#endif /* _USE_WRITE == 1 */
#if (1U == _USE_IOCTL)
	DRESULT USER_ioctl(BYTE _pdrv, BYTE _cmd, void *_buff);
#endif /* _USE_IOCTL == 1 */

Diskio_drvTypeDef FF_Driver = {
	USER_initialize,
	USER_status,
	USER_read,
#if  _USE_WRITE
	USER_write,
#endif  /* _USE_WRITE == 1 */
#if (1U == _USE_IOCTL)
	USER_ioctl,
#endif /* _USE_IOCTL == 1 */
};

/**
  ***************************************************************************************************************************************
  * @brief  Initializes a Drive
  * @param  pdrv: Physical drive number (0..)
  * @retval DSTATUS: Operation status
  ***************************************************************************************************************************************
  */
DSTATUS USER_initialize(BYTE _pdrv)
{
	if(BSP_QSPI_Get_Lock_Flag()){Stat |= STA_NOINIT;}
	else if(BSP_QSPI_Get_Init_Flag()){Stat &= ~STA_NOINIT;}
	else if(BSP_QSPI_Init() != QSPI_OK){Stat |= STA_NOINIT;}
	else{Stat &= ~STA_NOINIT;}

    return Stat;
}

/**
  ***************************************************************************************************************************************
  * @brief  Gets Disk Status
  * @param  pdrv: Physical drive number (0..)
  * @retval DSTATUS: Operation status
  ***************************************************************************************************************************************
  */
DSTATUS USER_status(BYTE _pdrv)
{
	if(!BSP_QSPI_Get_Init_Flag())
	{
		if(QSPI_OK == BSP_QSPI_Init()){Stat &= ~STA_NOINIT;}
		else{Stat |= STA_NOINIT;}
	}
	else if(BSP_QSPI_Get_Lock_Flag()){Stat |= STA_NOINIT;}
	else{Stat &= ~STA_NOINIT;}

	return Stat;
}

/**
  ***************************************************************************************************************************************
  * @brief  Reads Sector(s)
  * @param  pdrv: Physical drive number (0..)
  * @param  *buff: Data buffer to store read data
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to read (1..128)
  * @retval DRESULT: Operation result
  ***************************************************************************************************************************************
  */
DRESULT USER_read(BYTE _pdrv, BYTE *_buff, DWORD _sector, UINT _count)
{
	uint32_t _bufferSize = (DISKIO_BLK_SIZ * _count);
	uint32_t _address = (_sector * DISKIO_BLK_SIZ);

	if(BSP_QSPI_Get_Lock_Flag()){return RES_ERROR;}

	if(QSPI_OK != BSP_QSPI_Read(_buff, _address, _bufferSize)){return RES_ERROR;}

	return RES_OK;
}

/**
  ***************************************************************************************************************************************
  * @brief  Writes Sector(s)
  * @param  pdrv: Physical drive number (0..)
  * @param  *buff: Data to be written
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to write (1..128)
  * @retval DRESULT: Operation result
  ***************************************************************************************************************************************
  */
#if (1U == _USE_WRITE)
DRESULT USER_write(BYTE _pdrv, const BYTE *_buff, DWORD _sector, UINT _count)
{
	uint32_t _subsectorAddr = (_sector * DISKIO_BLK_SIZ);
	uint32_t _bufferSize = (DISKIO_BLK_SIZ * _count);
	uint32_t _address = (_sector * DISKIO_BLK_SIZ);

	if(BSP_QSPI_Get_Lock_Flag()){return RES_ERROR;}

	for(uint32_t _i = 0U; _i < _count; _i++)
	{
		if(QSPI_OK != BSP_QSPI_Erase_Block(_subsectorAddr)){return RES_ERROR;}
		_subsectorAddr += DISKIO_BLK_SIZ;
	}

	if(QSPI_OK != BSP_QSPI_Write((uint8_t*)_buff, _address, _bufferSize)){return RES_ERROR;}

	return RES_OK;
}
#endif /* _USE_WRITE == 1 */

/**
  ***************************************************************************************************************************************
  * @brief  I/O control operation
  * @param  pdrv: Physical drive number (0..)
  * @param  cmd: Control code
  * @param  *buff: Buffer to send/receive control data
  * @retval DRESULT: Operation result
  ***************************************************************************************************************************************
  */
#if (1U == _USE_IOCTL)
DRESULT USER_ioctl(BYTE _pdrv, BYTE _cmd, void *_buff)
{
	DRESULT _res = RES_ERROR;

	if(Stat & STA_NOINIT){return RES_NOTRDY;}

	switch(_cmd)
	{
		/* Make sure that no pending write process */
	  	case CTRL_SYNC : _res = RES_OK; break;

	  	/* Get number of sectors on the disk (DWORD) */
	  	case GET_SECTOR_COUNT :
	  		*(DWORD*)_buff = DISKIO_BLK_NBR;
	  		_res = RES_OK;
	    break;

	    /* Get R/W sector size (WORD) */
	  	case GET_SECTOR_SIZE :
	  		*(WORD*)_buff = DISKIO_BLK_SIZ;
	  		_res = RES_OK;
	    break;

	    /* Get erase block size in unit of sector (DWORD) */
	  	case GET_BLOCK_SIZE :
	  		*(DWORD*)_buff = DISKIO_BLK_SIZ;
	  		_res = RES_OK;
	    break;

	  	default: _res = RES_PARERR; break;
	  }

	  return _res;
}
#endif /* _USE_IOCTL == 1 */
