/**
  ******************************************************************************
  * @file    n25q512a_qspi.c
  * @author  MCD Application Team
  * @brief   This file includes a standard driver for the N25Q256A QSPI 
  *          memory mounted on STM32L476G-EVAL board.
  @verbatim
  ==============================================================================
                     ##### How to use this driver #####
  ==============================================================================  
  [..] 
   (#) This driver is used to drive the N25Q256A QSPI external 
       memory mounted on STM32L476G-EVAL evaluation board.
       
   (#) This driver need a specific component driver (N25Q256A) to be included with.

   (#) Initialization steps:
       (++) Initialize the QPSI external memory using the BSP_QSPI_Init() function. This 
            function includes the MSP layer hardware resources initialization and the
            QSPI interface with the external memory. The BSP_QSPI_DeInit() can be used 
            to deactivate the QSPI interface.
  
   (#) QSPI memory operations
       (++) QSPI memory can be accessed with read/write operations once it is
            initialized.
            Read/write operation can be performed with AHB access using the functions
            BSP_QSPI_Read()/BSP_QSPI_Write(). 
       (++) The function to the QSPI memory in memory-mapped mode is possible after 
            the call of the function BSP_QSPI_EnableMemoryMappedMode().
       (++) The function BSP_QSPI_GetInfo() returns the configuration of the QSPI memory. 
            (see the QSPI memory data sheet)
       (++) Perform erase block operation using the function BSP_QSPI_Erase_Block() and by
            specifying the block address. You can perform an erase operation of the whole 
            chip by calling the function BSP_QSPI_Erase_Chip(). 
       (++) The function BSP_QSPI_GetStatus() returns the current status of the QSPI memory. 
            (see the QSPI memory data sheet)
  @endverbatim
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

/* Includes ------------------------------------------------------------------*/
#include "n25q512a_qspi.h"

/** @addtogroup BSP
  * @{
  */

/** @addtogroup STM32L476G_EVAL
  * @{
  */

/** @defgroup STM32L476G_EVAL_QSPI STM32L476G_EVAL QSPI
  * @{
  */

/* Private variables ---------------------------------------------------------*/

/** @defgroup STM32L476G_EVAL_QSPI_Private_Variables Private Variables
  * @{
  */
QSPI_HandleTypeDef QSPIHandle;
__IO uint8_t qspiLockFlag=0;
__IO uint8_t qspiInitFlag=0;

/**
  * @}
  */

/* Private function prototypes -----------------------------------------------*/

/** @defgroup STM32L476G_EVAL_QSPI_Private_Functions Private Functions
  * @{
  */
static void    QSPI_MspInit              (void);
static void    QSPI_MspDeInit          	 (void);
static uint8_t QSPI_ResetMemory          (QSPI_HandleTypeDef *hqspi);
static uint8_t QSPI_EnterFourBytesAddress(QSPI_HandleTypeDef *hqspi);
static uint8_t QSPI_DummyCyclesCfg       (QSPI_HandleTypeDef *hqspi);
static uint8_t QSPI_WriteEnable          (QSPI_HandleTypeDef *hqspi);
static uint8_t QSPI_AutoPollingMemReady(QSPI_HandleTypeDef *hqspi, uint32_t Timeout);

/**
  * @}
  */

/* Exported functions ---------------------------------------------------------*/

/** @addtogroup STM32L476G_EVAL_QSPI_Exported_Functions
  * @{
  */

/**
  * @brief  Initializes the QSPI interface.
  * @retval QSPI memory status
  */
uint8_t BSP_QSPI_Init(void)
{ 
	qspiLockFlag++;

	QSPIHandle.Instance = QUADSPI;

	/* Call the DeInit function to reset the driver */
	if (HAL_QSPI_DeInit(&QSPIHandle) != HAL_OK){return QSPI_ERROR;}
        
	/* System level initialization */
	QSPI_MspInit();
  
	/* QSPI initialization */
	QSPIHandle.Init.ClockPrescaler     = 1; /* QSPI clock = 80MHz / (ClockPrescaler+1) = 40MHz */
	QSPIHandle.Init.FifoThreshold      = 1;
  	QSPIHandle.Init.SampleShifting     = QSPI_SAMPLE_SHIFTING_HALFCYCLE;
  	QSPIHandle.Init.FlashSize          = POSITION_VAL(N25Q512A_FLASH_SIZE) - 1;
  	QSPIHandle.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_5_CYCLE;
  	QSPIHandle.Init.ClockMode          = QSPI_CLOCK_MODE_0;
  	QSPIHandle.Init.FlashID            = QSPI_FLASH_ID_1;
  	QSPIHandle.Init.DualFlash          = QSPI_DUALFLASH_DISABLE;

  	if (HAL_QSPI_Init(&QSPIHandle) != HAL_OK)
  	{
  		if(qspiLockFlag){qspiLockFlag--;}
  		return QSPI_ERROR;
  	}

  	/* QSPI memory reset */
  	if (QSPI_ResetMemory(&QSPIHandle) != QSPI_OK)
  	{
  		if(qspiLockFlag){qspiLockFlag--;}
  		return QSPI_NOT_SUPPORTED;
  	}
 
  	/* Set the QSPI memory in 4-bytes address mode */
  	if (QSPI_EnterFourBytesAddress(&QSPIHandle) != QSPI_OK)
  	{
  		if(qspiLockFlag){qspiLockFlag--;}
  		return QSPI_NOT_SUPPORTED;
  	}
 
  	/* Configuration of the dummy cucles on QSPI memory side */
  	if (QSPI_DummyCyclesCfg(&QSPIHandle) != QSPI_OK)
  	{
  		if(qspiLockFlag){qspiLockFlag--;}
  		return QSPI_NOT_SUPPORTED;
  	}
  
  	qspiInitFlag=1;
  	if(qspiLockFlag){qspiLockFlag--;}
  	return QSPI_OK;
}

/**
  * @brief  Get QSPI Init flag
  * @retval Init flag
  */
uint8_t BSP_QSPI_Get_Init_Flag(void)
{
	return qspiInitFlag;
}

/**
  * @brief  Get QSPI lock flag
  * @retval Lock flag
  */
uint8_t BSP_QSPI_Get_Lock_Flag(void)
{
	return qspiLockFlag;
}

/**
  * @brief  De-Initializes the QSPI interface.
  * @retval QSPI memory status
  */
uint8_t BSP_QSPI_DeInit(void)
{ 
	qspiLockFlag++;

	QSPIHandle.Instance = QUADSPI;

	/* Call the DeInit function to reset the driver */
	if (HAL_QSPI_DeInit(&QSPIHandle) != HAL_OK)
	{
		if(qspiLockFlag){qspiLockFlag--;}
		return QSPI_ERROR;
	}
        
	/* System level De-initialization */
	QSPI_MspDeInit();
  
	if(qspiLockFlag){qspiLockFlag--;}
	return QSPI_OK;
}

/**
  * @brief  Reads an amount of data from the QSPI memory.
  * @param  pData: Pointer to data to be read
  * @param  ReadAddr: Read start address
  * @param  Size: Size of data to read    
  * @retval QSPI memory status
  */
uint8_t BSP_QSPI_Read(uint8_t* pData, uint32_t ReadAddr, uint32_t Size)
{
	QSPI_CommandTypeDef sCommand;

	qspiLockFlag++;
	/* Initialize the read command */
	sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
	sCommand.Instruction       = QUAD_OUT_FAST_READ_CMD;
	sCommand.AddressMode       = QSPI_ADDRESS_1_LINE;
	sCommand.AddressSize       = QSPI_ADDRESS_32_BITS;
	sCommand.Address           = ReadAddr;
	sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	sCommand.DataMode          = QSPI_DATA_4_LINES;
	sCommand.DummyCycles       = N25Q512A_DUMMY_CYCLES_READ_QUAD;
	sCommand.NbData            = Size;
	sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
	sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
	sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
  
	/* Configure the command */
	if (HAL_QSPI_Command(&QSPIHandle, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		if(qspiLockFlag){qspiLockFlag--;}
		return QSPI_ERROR;
	}
  
	MODIFY_REG(QSPIHandle.Instance->DCR, QUADSPI_DCR_CSHT, QSPI_CS_HIGH_TIME_2_CYCLE);

	/* Reception of the data */
	if (HAL_QSPI_Receive(&QSPIHandle, pData, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		if(qspiLockFlag){qspiLockFlag--;}
		return QSPI_ERROR;
	}

	MODIFY_REG(QSPIHandle.Instance->DCR, QUADSPI_DCR_CSHT, QSPI_CS_HIGH_TIME_5_CYCLE);

	if(qspiLockFlag){qspiLockFlag--;}
	return QSPI_OK;
}

/**
  * @brief  Writes an amount of data to the QSPI memory.
  * @param  pData: Pointer to data to be written
  * @param  WriteAddr: Write start address
  * @param  Size: Size of data to write    
  * @retval QSPI memory status
  */
uint8_t BSP_QSPI_Write(uint8_t* pData, uint32_t WriteAddr, uint32_t Size)
{
	QSPI_CommandTypeDef sCommand;
	uint32_t end_addr, current_size, current_addr;

	qspiLockFlag++;
	/* Calculation of the size between the write address and the end of the page */
	current_size = N25Q512A_PAGE_SIZE - (WriteAddr % N25Q512A_PAGE_SIZE);

	/* Check if the size of the data is less than the remaining place in the page */
	if (current_size > Size){current_size = Size;}

	/* Initialize the adress variables */
	current_addr = WriteAddr;
	end_addr = WriteAddr + Size;

	/* Initialize the program command */
	sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
	sCommand.Instruction       = QUAD_IN_FAST_PROG_CMD;
	sCommand.AddressMode       = QSPI_ADDRESS_1_LINE;
	sCommand.AddressSize       = QSPI_ADDRESS_32_BITS;
	sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	sCommand.DataMode          = QSPI_DATA_4_LINES;
	sCommand.DummyCycles       = 0;
	sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
	sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
	sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
  
	/* Perform the write page by page */
	do
	{
		sCommand.Address = current_addr;
		sCommand.NbData  = current_size;

		/* Enable write operations */
		if (QSPI_WriteEnable(&QSPIHandle) != QSPI_OK)
		{
			if(qspiLockFlag){qspiLockFlag--;}
			return QSPI_ERROR;
		}
    
		/* Configure the command */
		if (HAL_QSPI_Command(&QSPIHandle, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		{
			if(qspiLockFlag){qspiLockFlag--;}
			return QSPI_ERROR;
		}
    
		/* Transmission of the data */
		if (HAL_QSPI_Transmit(&QSPIHandle, pData, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		{
			if(qspiLockFlag){qspiLockFlag--;}
			return QSPI_ERROR;
		}
    
		/* Configure automatic polling mode to wait for end of program */
		if (QSPI_AutoPollingMemReady(&QSPIHandle, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != QSPI_OK)
		{
			if(qspiLockFlag){qspiLockFlag--;}
			return QSPI_ERROR;
		}
    
		/* Update the address and size variables for next page programming */
		current_addr += current_size;
		pData += current_size;
		current_size = ((current_addr + N25Q512A_PAGE_SIZE) > end_addr) ? (end_addr - current_addr) : N25Q512A_PAGE_SIZE;
	} while (current_addr < end_addr);
  
	if(qspiLockFlag){qspiLockFlag--;}
	return QSPI_OK;
}

/**
  * @brief  Erases the specified block of the QSPI memory. 
  * @param  BlockAddress: Block address to erase  
  * @retval QSPI memory status
  */
uint8_t BSP_QSPI_Erase_Block(uint32_t BlockAddress)
{
	QSPI_CommandTypeDef sCommand;

	qspiLockFlag++;
	/* Initialize the erase command */
	sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
	sCommand.Instruction       = SUBSECTOR_ERASE_CMD;
	sCommand.AddressMode       = QSPI_ADDRESS_1_LINE;
	sCommand.AddressSize       = QSPI_ADDRESS_32_BITS;
	sCommand.Address           = BlockAddress;
	sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	sCommand.DataMode          = QSPI_DATA_NONE;
	sCommand.DummyCycles       = 0;
	sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
	sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
	sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

	/* Enable write operations */
	if (QSPI_WriteEnable(&QSPIHandle) != QSPI_OK)
	{
		if(qspiLockFlag){qspiLockFlag--;}
		return QSPI_ERROR;
	}

	/* Send the command */
	if (HAL_QSPI_Command(&QSPIHandle, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		if(qspiLockFlag){qspiLockFlag--;}
		return QSPI_ERROR;
	}
  
	/* Configure automatic polling mode to wait for end of erase */
	if (QSPI_AutoPollingMemReady(&QSPIHandle, N25Q512A_SUBSECTOR_ERASE_MAX_TIME) != QSPI_OK)
	{
		if(qspiLockFlag){qspiLockFlag--;}
		return QSPI_ERROR;
	}

	if(qspiLockFlag){qspiLockFlag--;}
	return QSPI_OK;
}

/**
  * @brief  Erases the entire QSPI memory.
  * @retval QSPI memory status
  */
uint8_t BSP_QSPI_Erase_Chip(void)
{
	QSPI_CommandTypeDef sCommand;

	qspiLockFlag++;
	/* Initialize the erase command */
	sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
	sCommand.Instruction       = BULK_ERASE_CMD;
	sCommand.AddressMode       = QSPI_ADDRESS_NONE;
	sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	sCommand.DataMode          = QSPI_DATA_NONE;
	sCommand.DummyCycles       = 0;
	sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
	sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
	sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

	/* Enable write operations */
	if (QSPI_WriteEnable(&QSPIHandle) != QSPI_OK)
	{
		if(qspiLockFlag){qspiLockFlag--;}
		return QSPI_ERROR;
	}

	/* Send the command */
	if (HAL_QSPI_Command(&QSPIHandle, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		if(qspiLockFlag){qspiLockFlag--;}
		return QSPI_ERROR;
	}
  
	/* Configure automatic polling mode to wait for end of erase */
	if (QSPI_AutoPollingMemReady(&QSPIHandle, N25Q512A_BULK_ERASE_MAX_TIME) != QSPI_OK)
	{
		if(qspiLockFlag){qspiLockFlag--;}
		return QSPI_ERROR;
	}

	if(qspiLockFlag){qspiLockFlag--;}
	return QSPI_OK;
}

/**
  * @brief  Reads current status of the QSPI memory.
  * @retval QSPI memory status
  */
uint8_t BSP_QSPI_GetStatus(void)
{
	QSPI_CommandTypeDef sCommand;
	uint8_t reg;

	qspiLockFlag++;
	/* Initialize the read flag status register command */
	sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
	sCommand.Instruction       = READ_FLAG_STATUS_REG_CMD;
	sCommand.AddressMode       = QSPI_ADDRESS_NONE;
	sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	sCommand.DataMode          = QSPI_DATA_1_LINE;
	sCommand.DummyCycles       = 0;
	sCommand.NbData            = 1;
	sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
	sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
	sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

	/* Configure the command */
	if (HAL_QSPI_Command(&QSPIHandle, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		if(qspiLockFlag){qspiLockFlag--;}
		return QSPI_ERROR;
	}

	/* Reception of the data */
	if (HAL_QSPI_Receive(&QSPIHandle, &reg, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		if(qspiLockFlag){qspiLockFlag--;}
		return QSPI_ERROR;
	}
  
	/* Check the value of the register */
	if ((reg & (N25Q512A_FSR_PRERR | N25Q512A_FSR_VPPERR | N25Q512A_FSR_PGERR | N25Q512A_FSR_ERERR)) != 0)
	{
		if(qspiLockFlag){qspiLockFlag--;}
		return QSPI_ERROR;
	}
	else if ((reg & (N25Q512A_FSR_PGSUS | N25Q512A_FSR_ERSUS)) != 0)
	{
		if(qspiLockFlag){qspiLockFlag--;}
		return QSPI_SUSPENDED;
	}
	else if ((reg & N25Q512A_FSR_READY) != 0)
	{
		if(qspiLockFlag){qspiLockFlag--;}
		return QSPI_OK;
	}
	else
	{
		if(qspiLockFlag){qspiLockFlag--;}
		return QSPI_BUSY;
	}
}

/**
  * @brief  Return the configuration of the QSPI memory.
  * @param  pInfo: pointer on the configuration structure  
  * @retval QSPI memory status
  */
uint8_t BSP_QSPI_GetInfo(QSPI_Info* pInfo)
{
	/* Configure the structure with the memory configuration */
	pInfo->FlashSize          = N25Q512A_FLASH_SIZE;
	pInfo->EraseSectorSize    = N25Q512A_SUBSECTOR_SIZE;
	pInfo->EraseSectorsNumber = (N25Q512A_FLASH_SIZE/N25Q512A_SUBSECTOR_SIZE);
	pInfo->ProgPageSize       = N25Q512A_PAGE_SIZE;
	pInfo->ProgPagesNumber    = (N25Q512A_FLASH_SIZE/N25Q512A_PAGE_SIZE);
  
	return QSPI_OK;
}

/**
  * @brief  Configure the QSPI in memory-mapped mode
  * @retval QSPI memory status
  */
uint8_t BSP_QSPI_EnableMemoryMappedMode(void)
{
	QSPI_CommandTypeDef      sCommand;
	QSPI_MemoryMappedTypeDef sMemMappedCfg;

	qspiLockFlag++;
	/* Configure the command for the read instruction */
	sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
	sCommand.Instruction       = QUAD_OUT_FAST_READ_CMD;
	sCommand.AddressMode       = QSPI_ADDRESS_1_LINE;
	sCommand.AddressSize       = QSPI_ADDRESS_32_BITS;
	sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	sCommand.DataMode          = QSPI_DATA_4_LINES;
	sCommand.DummyCycles       = N25Q512A_DUMMY_CYCLES_READ_QUAD;
	sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
	sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
	sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
  
	/* Configure the memory mapped mode */
	sMemMappedCfg.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;
  
	if (HAL_QSPI_MemoryMapped(&QSPIHandle, &sCommand, &sMemMappedCfg) != HAL_OK)
	{
		if(qspiLockFlag){qspiLockFlag--;}
		return QSPI_ERROR;
	}

	if(qspiLockFlag){qspiLockFlag--;}
	return QSPI_OK;
}

/**
  * @}
  */

/** @addtogroup STM32L476G_EVAL_QSPI_Private_Functions 
  * @{
  */

/**
  * @brief  Initializes the QSPI MSP.
  * @retval None
  */
static void QSPI_MspInit(void)
{
  	GPIO_InitTypeDef gpio_init_structure;

  	/*##-1- Enable peripherals and GPIO Clocks #################################*/
  	/* Enable the QuadSPI memory interface clock */
  	QSPI_CLK_ENABLE();
  	QSPI_CS_GPIOA_CLK_ENABLE();
  	QSPI_CS_GPIOB_CLK_ENABLE();
  	/* Reset the QuadSPI memory interface */
  	QSPI_FORCE_RESET();
  	QSPI_RELEASE_RESET();

    gpio_init_structure.Mode = GPIO_MODE_AF_PP;
    gpio_init_structure.Pull = GPIO_NOPULL;
    gpio_init_structure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init_structure.Alternate = GPIO_AF10_QUADSPI;
    gpio_init_structure.Pin = QSPI_CS_PIN;
    HAL_GPIO_Init(QSPI_CS_GPIO_PORT, &gpio_init_structure);
    gpio_init_structure.Pin = QSPI_CLK_PIN;
    HAL_GPIO_Init(QSPI_CLK_GPIO_PORT, &gpio_init_structure);
    gpio_init_structure.Pin = QSPI_D0_PIN|QSPI_D1_PIN;
    HAL_GPIO_Init(QSPI_DB_GPIO_PORT, &gpio_init_structure);
    gpio_init_structure.Pin = QSPI_D2_PIN|QSPI_D3_PIN;
    HAL_GPIO_Init(QSPI_DA_GPIO_PORT, &gpio_init_structure);
}

/**
  * @brief  De-Initializes the QSPI MSP.
  * @retval None
  */
static void QSPI_MspDeInit(void)
{
	/*##-2- Disable peripherals and GPIO Clocks ################################*/
	/* De-Configure QSPI pins */
	HAL_GPIO_DeInit(QSPI_CS_GPIO_PORT, QSPI_CS_PIN);
	HAL_GPIO_DeInit(QSPI_CLK_GPIO_PORT, QSPI_CLK_PIN);
	HAL_GPIO_DeInit(QSPI_DB_GPIO_PORT, QSPI_D0_PIN);
	HAL_GPIO_DeInit(QSPI_DB_GPIO_PORT, QSPI_D1_PIN);
	HAL_GPIO_DeInit(QSPI_DA_GPIO_PORT, QSPI_D2_PIN);
	HAL_GPIO_DeInit(QSPI_DA_GPIO_PORT, QSPI_D3_PIN);

	/*##-3- Reset peripherals ##################################################*/
	/* Reset the QuadSPI memory interface */
	QSPI_FORCE_RESET();
	QSPI_RELEASE_RESET();

	/* Disable the QuadSPI memory interface clock */
	QSPI_CLK_DISABLE();
}

/**
  * @brief  This function reset the QSPI memory.
  * @param  hqspi: QSPI handle
  * @retval None
  */
static uint8_t QSPI_ResetMemory(QSPI_HandleTypeDef *hqspi)
{
	QSPI_CommandTypeDef sCommand;

	/* Initialize the reset enable command */
	sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
	sCommand.Instruction       = RESET_ENABLE_CMD;
	sCommand.AddressMode       = QSPI_ADDRESS_NONE;
	sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	sCommand.DataMode          = QSPI_DATA_NONE;
	sCommand.DummyCycles       = 0;
	sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
	sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
	sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

	/* Send the command */
	if (HAL_QSPI_Command(&QSPIHandle, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK){return QSPI_ERROR;}

	/* Send the reset memory command */
	sCommand.Instruction = RESET_MEMORY_CMD;
	if (HAL_QSPI_Command(&QSPIHandle, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK){return QSPI_ERROR;}

	/* Configure automatic polling mode to wait the memory is ready */
	if (QSPI_AutoPollingMemReady(&QSPIHandle, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != QSPI_OK){return QSPI_ERROR;}

	return QSPI_OK;
}

/**
  * @brief  This function set the QSPI memory in 4-byte address mode
  * @param  hqspi: QSPI handle
  * @retval None
  */
static uint8_t QSPI_EnterFourBytesAddress(QSPI_HandleTypeDef *hqspi)
{
	QSPI_CommandTypeDef sCommand;

	/* Initialize the command */
	sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
	sCommand.Instruction       = ENTER_4_BYTE_ADDR_MODE_CMD;
	sCommand.AddressMode       = QSPI_ADDRESS_NONE;
	sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	sCommand.DataMode          = QSPI_DATA_NONE;
	sCommand.DummyCycles       = 0;
	sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
	sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
	sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

	/* Enable write operations */
	if (QSPI_WriteEnable(&QSPIHandle) != QSPI_OK){return QSPI_ERROR;}

	/* Send the command */
	if (HAL_QSPI_Command(&QSPIHandle, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK){return QSPI_ERROR;}

	/* Configure automatic polling mode to wait the memory is ready */
	if (QSPI_AutoPollingMemReady(&QSPIHandle, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != QSPI_OK){return QSPI_ERROR;}

	return QSPI_OK;
}

/**
  * @brief  This function configure the dummy cycles on memory side.
  * @param  hqspi: QSPI handle
  * @retval None
  */
static uint8_t QSPI_DummyCyclesCfg(QSPI_HandleTypeDef *hqspi)
{
	QSPI_CommandTypeDef sCommand;
	uint8_t reg;

	/* Initialize the read volatile configuration register command */
	sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
	sCommand.Instruction       = READ_VOL_CFG_REG_CMD;
	sCommand.AddressMode       = QSPI_ADDRESS_NONE;
	sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	sCommand.DataMode          = QSPI_DATA_1_LINE;
	sCommand.DummyCycles       = 0;
	sCommand.NbData            = 1;
	sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
	sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
	sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

	/* Configure the command */
	if (HAL_QSPI_Command(&QSPIHandle, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK){return QSPI_ERROR;}

	/* Reception of the data */
	if (HAL_QSPI_Receive(&QSPIHandle, &reg, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK){return QSPI_ERROR;}

	/* Enable write operations */
	if (QSPI_WriteEnable(&QSPIHandle) != QSPI_OK){return QSPI_ERROR;}

	/* Update volatile configuration register (with new dummy cycles) */
	sCommand.Instruction = WRITE_VOL_CFG_REG_CMD;
	MODIFY_REG(reg, N25Q512A_VCR_NB_DUMMY, (N25Q512A_DUMMY_CYCLES_READ_QUAD << POSITION_VAL(N25Q512A_VCR_NB_DUMMY)));
      
	/* Configure the write volatile configuration register command */
	if (HAL_QSPI_Command(&QSPIHandle, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK){return QSPI_ERROR;}

	/* Transmission of the data */
	if (HAL_QSPI_Transmit(&QSPIHandle, &reg, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK){return QSPI_ERROR;}
  
	return QSPI_OK;
}

/**
  * @brief  This function send a Write Enable and wait it is effective.
  * @param  hqspi: QSPI handle
  * @retval None
  */
static uint8_t QSPI_WriteEnable(QSPI_HandleTypeDef *hqspi)
{
	QSPI_CommandTypeDef     sCommand;
	QSPI_AutoPollingTypeDef sConfig;

	/* Enable write operations */
	sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
	sCommand.Instruction       = WRITE_ENABLE_CMD;
	sCommand.AddressMode       = QSPI_ADDRESS_NONE;
	sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	sCommand.DataMode          = QSPI_DATA_NONE;
	sCommand.DummyCycles       = 0;
	sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
	sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
	sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

	if (HAL_QSPI_Command(&QSPIHandle, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK){return QSPI_ERROR;}
  
	/* Configure automatic polling mode to wait for write enabling */
	sConfig.Match           = N25Q512A_SR_WREN;
	sConfig.Mask            = N25Q512A_SR_WREN;
	sConfig.MatchMode       = QSPI_MATCH_MODE_AND;
	sConfig.StatusBytesSize = 1;
	sConfig.Interval        = 0x10;
	sConfig.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;

	sCommand.Instruction    = READ_STATUS_REG_CMD;
	sCommand.DataMode       = QSPI_DATA_1_LINE;

	if (HAL_QSPI_AutoPolling(&QSPIHandle, &sCommand, &sConfig, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK){return QSPI_ERROR;}

	return QSPI_OK;
}

/**
  * @brief  This function read the SR of the memory and wait the EOP.
  * @param  hqspi: QSPI handle
  * @param  Timeout: Timeout for auto-polling
  * @retval None
  */
static uint8_t QSPI_AutoPollingMemReady(QSPI_HandleTypeDef *hqspi, uint32_t Timeout)
{
	QSPI_CommandTypeDef     sCommand;
	QSPI_AutoPollingTypeDef sConfig;

	/* Configure automatic polling mode to wait for memory ready */
	sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
	sCommand.Instruction       = READ_STATUS_REG_CMD;
	sCommand.AddressMode       = QSPI_ADDRESS_NONE;
	sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	sCommand.DataMode          = QSPI_DATA_1_LINE;
	sCommand.DummyCycles       = 0;
	sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
	sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
	sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

	sConfig.Match           = 0;
	sConfig.Mask            = N25Q512A_SR_WIP;
  	sConfig.MatchMode       = QSPI_MATCH_MODE_AND;
  	sConfig.StatusBytesSize = 1;
  	sConfig.Interval        = 0x10;
  	sConfig.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;

  	if (HAL_QSPI_AutoPolling(&QSPIHandle, &sCommand, &sConfig, Timeout) != HAL_OK){return QSPI_ERROR;}

  	return QSPI_OK;
}

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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
