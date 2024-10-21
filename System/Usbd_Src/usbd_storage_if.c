/**
  ***************************************************************************************************************************************
  * @file     usbd_storage_if.c
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

#include "usbd_storage_if.h"
#include "n25q512a_qspi.h"

#define STORAGE_LUN_NBR                  1
#define STORAGE_BLK_NBR                  0x4000
#define STORAGE_BLK_SIZ                  0x1000

/* USB Mass storage Standard Inquiry Data */
const int8_t STORAGE_Inquirydata_FS[]={
	/* LUN 0 */
	0x00,
	0x80,
	0x02,
	0x02,
	(STANDARD_INQUIRY_DATA_LEN - 5),
	0x00,
	0x00,
	0x00,
	'S', 'i', 'm', 'o', 'n', 'B', 'a', 't', /* Manufacturer : 8 bytes */
	'S', 'e', 'n', 't', 'i', 'n', 'e', 'l', /* Product      : 16 Bytes */
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
	'0', '.', '0' ,'1'                      /* Version      : 4 Bytes */
};

extern USBD_HandleTypeDef hUsbDeviceFS;

static int8_t STORAGE_Init_FS(uint8_t lun);
static int8_t STORAGE_GetCapacity_FS(uint8_t lun, uint32_t *block_num, uint16_t *block_size);
static int8_t STORAGE_IsReady_FS(uint8_t lun);
static int8_t STORAGE_IsWriteProtected_FS(uint8_t lun);
static int8_t STORAGE_Read_FS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len);
static int8_t STORAGE_Write_FS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len);
static int8_t STORAGE_GetMaxLun_FS(void);

USBD_StorageTypeDef USBD_Storage_Interface_fops_FS={
	STORAGE_Init_FS,
	STORAGE_GetCapacity_FS,
	STORAGE_IsReady_FS,
	STORAGE_IsWriteProtected_FS,
	STORAGE_Read_FS,
	STORAGE_Write_FS,
	STORAGE_GetMaxLun_FS,
	(int8_t *)STORAGE_Inquirydata_FS
};

/**
  ***************************************************************************************************************************************
  * @brief  Initializes over USB FS IP
  * @param  lun:
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  ***************************************************************************************************************************************
  */
int8_t STORAGE_Init_FS(uint8_t lun)
{
	if(BSP_QSPI_Get_Lock_Flag()){return USBD_FAIL;}

	if(BSP_QSPI_Get_Init_Flag()){return USBD_OK;}
	else if(QSPI_OK != BSP_QSPI_Init()){return USBD_FAIL;}

	return USBD_OK;
}

/**
  ***************************************************************************************************************************************
  * @brief  Get capacity
  * @param  lun:
  * @param  block_num:
  * @param  block_size:
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  ***************************************************************************************************************************************
  */
int8_t STORAGE_GetCapacity_FS(uint8_t lun, uint32_t *block_num, uint16_t *block_size)
{
	*block_num  = STORAGE_BLK_NBR;
	*block_size = STORAGE_BLK_SIZ;

	return USBD_OK;
}

/**
  ***************************************************************************************************************************************
  * @brief  Storage is ready
  * @param  lun:
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  ***************************************************************************************************************************************
  */
int8_t STORAGE_IsReady_FS(uint8_t lun)
{
	if(!BSP_QSPI_Get_Init_Flag())
	{
		if(QSPI_OK == BSP_QSPI_Init()){return USBD_OK;}
		else{return USBD_FAIL;}
	}
	else if(BSP_QSPI_Get_Lock_Flag()){return USBD_FAIL;}

	return USBD_OK;
}

/**
  ***************************************************************************************************************************************
  * @brief  Storage is ready protected
  * @param  lun:
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  ***************************************************************************************************************************************
  */
int8_t STORAGE_IsWriteProtected_FS(uint8_t lun)
{
	return USBD_OK;
}

/**
  ***************************************************************************************************************************************
  * @brief  Storage read
  * @param  lun:
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  ***************************************************************************************************************************************
  */
int8_t STORAGE_Read_FS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len)
{
	uint32_t bufferSize = (STORAGE_BLK_SIZ * blk_len);
	uint32_t address = (blk_addr * STORAGE_BLK_SIZ);

	if(BSP_QSPI_Get_Lock_Flag()){return USBD_FAIL;}

	if(QSPI_OK != BSP_QSPI_Read(buf,address,bufferSize)){return USBD_FAIL;}

	return USBD_OK;
}

/**
  ***************************************************************************************************************************************
  * @brief  Storage write
  * @param  lun:
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  ***************************************************************************************************************************************
  */
int8_t STORAGE_Write_FS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len)
{
	uint32_t subsectorAddr = (blk_addr * STORAGE_BLK_SIZ);
	uint32_t bufferSize = (STORAGE_BLK_SIZ * blk_len);
	uint32_t address = (blk_addr * STORAGE_BLK_SIZ);
	uint32_t i;

	if(BSP_QSPI_Get_Lock_Flag()){return USBD_FAIL;}

	for(i=0;i<blk_len;i++)
	{
		if(QSPI_OK != BSP_QSPI_Erase_Block(subsectorAddr)){return USBD_FAIL;}
		subsectorAddr += STORAGE_BLK_SIZ;
	}

	if(QSPI_OK != BSP_QSPI_Write(buf, address, bufferSize)){return USBD_FAIL;}

	return USBD_OK;
}

/**
  ***************************************************************************************************************************************
  * @brief  Get maximum LUN
  * @param  None
  * @retval LUN
  ***************************************************************************************************************************************
  */
int8_t STORAGE_GetMaxLun_FS(void)
{
	return (STORAGE_LUN_NBR - 1U);
}
