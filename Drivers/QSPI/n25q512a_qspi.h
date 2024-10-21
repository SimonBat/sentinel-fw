/**
  ******************************************************************************
  * @file    n25q512a_qspi.h
  * @author  MCD Application Team
  * @brief   This file contains the common defines and functions prototypes for
  *          the stm32l476g_eval_qspi.c driver.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2016 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32L476G_EVAL_QSPI_H
#define __STM32L476G_EVAL_QSPI_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"
#include "n25q512a.h"
#include "bsp.h"

/** @addtogroup BSP
  * @{
  */

/** @addtogroup STM32L476G_EVAL
  * @{
  */

/** @addtogroup STM32L476G_EVAL_QSPI
  * @{
  */

  
/* Exported constants --------------------------------------------------------*/ 
/** @defgroup STM32L476G_EVAL_QSPI_Exported_Constants Exported Constants
  * @{
  */
/* QSPI Error codes */
#define QSPI_OK            ((uint8_t)0x00)
#define QSPI_ERROR         ((uint8_t)0x01)
#define QSPI_BUSY          ((uint8_t)0x02)
#define QSPI_NOT_SUPPORTED ((uint8_t)0x04)
#define QSPI_SUSPENDED     ((uint8_t)0x08)

/* Definition for QSPI clock resources */
#define QSPI_CLK_ENABLE()             	__HAL_RCC_QSPI_CLK_ENABLE()
#define QSPI_CLK_DISABLE()            	__HAL_RCC_QSPI_CLK_DISABLE()
#define QSPI_CS_GPIOA_CLK_ENABLE()     	__HAL_RCC_GPIOA_CLK_ENABLE()
#define QSPI_CS_GPIOA_CLK_DISABLE()  	__HAL_RCC_GPIOA_CLK_DISABLE()
#define QSPI_CS_GPIOB_CLK_ENABLE()     	__HAL_RCC_GPIOB_CLK_ENABLE()
#define QSPI_CS_GPIOB_CLK_DISABLE()  	__HAL_RCC_GPIOB_CLK_DISABLE()
#define QSPI_FORCE_RESET()            	__HAL_RCC_QSPI_FORCE_RESET()
#define QSPI_RELEASE_RESET()          	__HAL_RCC_QSPI_RELEASE_RESET()

/* Definition for QSPI Pins */
#define QSPI_CS_PIN                	BSP_FLASH_NCS_PIN
#define QSPI_CS_GPIO_PORT          	BSP_FLASH_NCS_PORT
#define QSPI_CLK_PIN               	BSP_FLASH_CLK_PIN
#define QSPI_CLK_GPIO_PORT         	BSP_FLASH_CLK_PORT
#define QSPI_D0_PIN                	BSP_FLASH_IO0_PIN
#define QSPI_D1_PIN                	BSP_FLASH_IO1_PIN
#define QSPI_DB_GPIO_PORT			BSP_FLASH_IO0_PORT
#define QSPI_D2_PIN                	BSP_FLASH_IO2_PIN
#define QSPI_D3_PIN                	BSP_FLASH_IO3_PIN
#define QSPI_DA_GPIO_PORT          	BSP_FLASH_IO2_PORT

/**
  * @}
  */

/* Exported types ------------------------------------------------------------*/
/** @defgroup STM32L476G_EVAL_QSPI_Exported_Types Exported Types
  * @{
  */
/* QSPI Info */
typedef struct{
	uint32_t FlashSize;          /*!< Size of the flash */
	uint32_t EraseSectorSize;    /*!< Size of sectors for the erase operation */
	uint32_t EraseSectorsNumber; /*!< Number of sectors for the erase operation */
	uint32_t ProgPageSize;       /*!< Size of pages for the program operation */
	uint32_t ProgPagesNumber;    /*!< Number of pages for the program operation */
}QSPI_Info;

/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/
/** @defgroup STM32L476G_EVAL_QSPI_Exported_Functions Exported Functions
  * @{
  */
uint8_t BSP_QSPI_Init(void);
uint8_t BSP_QSPI_Get_Init_Flag(void);
uint8_t BSP_QSPI_Get_Lock_Flag(void);
uint8_t BSP_QSPI_DeInit(void);
uint8_t BSP_QSPI_Read(uint8_t* pData, uint32_t ReadAddr, uint32_t Size);
uint8_t BSP_QSPI_Write(uint8_t* pData, uint32_t WriteAddr, uint32_t Size);
uint8_t BSP_QSPI_Erase_Block(uint32_t BlockAddress);
uint8_t BSP_QSPI_Erase_Chip(void);
uint8_t BSP_QSPI_GetStatus(void);
uint8_t BSP_QSPI_GetInfo(QSPI_Info* pInfo);
uint8_t BSP_QSPI_EnableMemoryMappedMode(void);

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* __STM32L476G_EVAL_QSPI_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
