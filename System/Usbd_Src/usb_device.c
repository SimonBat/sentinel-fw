/**
  ***************************************************************************************************************************************
  * @file     usb_device.c
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

#include "usb_device.h"
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_msc.h"
#include "usbd_storage_if.h"
#include "bsp.h"

/* USB Device Core handle declaration */
USBD_HandleTypeDef hUsbDeviceFS;
extern USBD_DescriptorsTypeDef FS_Desc;

/**
  ***************************************************************************************************************************************
  * Init USB device Library, add supported class and start the library
  * @param None
  * @retval None
  ***************************************************************************************************************************************
  */
void USB_Device_Init(void)
{
	/* Init Device Library, add supported class and start the library */
	if (USBD_OK != USBD_Init(&hUsbDeviceFS, &FS_Desc, DEVICE_FS)){BSP_Error_Handler();}
	if (USBD_OK != USBD_RegisterClass(&hUsbDeviceFS, &USBD_MSC)){BSP_Error_Handler();}
	if (USBD_OK != USBD_MSC_RegisterStorage(&hUsbDeviceFS, &USBD_Storage_Interface_fops_FS)){BSP_Error_Handler();}
	if (USBD_OK != USBD_Start(&hUsbDeviceFS)){BSP_Error_Handler();}
}
