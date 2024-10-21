/**
 ***************************************************************************************************************************************
 * @file     i2c.c
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

#include "i2c.h"
#include "bsp.h"

static i2c_ts H_I2C;

/**
  ***************************************************************************************************************************************
  * @brief  I2C1 Initialization Function
  * @param  None
  * @retval None
  ***************************************************************************************************************************************
  */
void I2C_Driver_Init(void)
{
	H_I2C.hal.Instance = I2C1;
	H_I2C.hal.Init.Timing = 0x10801442;
	H_I2C.hal.Init.OwnAddress1 = 0U;
	H_I2C.hal.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	H_I2C.hal.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	H_I2C.hal.Init.OwnAddress2 = 0U;
	H_I2C.hal.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
	H_I2C.hal.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	H_I2C.hal.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	if(HAL_OK != HAL_I2C_Init(&H_I2C.hal)){BSP_Error_Handler();}
	/* Configure Analogue filter */
	if(HAL_OK != HAL_I2CEx_ConfigAnalogFilter(&H_I2C.hal, I2C_ANALOGFILTER_ENABLE)){BSP_Error_Handler();}
	/* Configure Digital filter */
	if(HAL_OK != HAL_I2CEx_ConfigDigitalFilter(&H_I2C.hal, 0U)){BSP_Error_Handler();}
	H_I2C.initFlag = 1U;
}

/**
  ***************************************************************************************************************************************
  * @brief I2C MSP Initialization
  * @param I2C handle (I2C_HandleTypeDef*)
  * @retval None
  ***************************************************************************************************************************************
  */
void HAL_I2C_MspInit(I2C_HandleTypeDef* _hi2c)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	if(I2C1 == _hi2c->Instance)
	{
		/* Peripheral clock enable */
		__HAL_RCC_I2C1_CLK_ENABLE();
		__HAL_RCC_GPIOA_CLK_ENABLE();
		/** I2C1 GPIO Configuration
    	  *	PA9 ------> I2C1_SCL
    	  *	PA10 ------> I2C1_SDA
    	  */
		GPIO_InitStruct.Pin = I2C_SCL_PIN | I2C_SDA_PIN;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
		GPIO_InitStruct.Pull = GPIO_PULLUP;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
		HAL_GPIO_Init(I2C_PORT, &GPIO_InitStruct);
	}
}

/**
  ***************************************************************************************************************************************
  * @brief I2C MSP De-Initialization
  * @param I2C handle (I2C_HandleTypeDef*)
  * @retval None
  ***************************************************************************************************************************************
  */
void HAL_I2C_MspDeInit(I2C_HandleTypeDef* _hi2c)
{
	if(I2C1 == _hi2c->Instance)
	{
		/* Peripheral clock disable */
		__HAL_RCC_I2C1_CLK_DISABLE();

		/** I2C1 GPIO Configuration
		  * PA9 ------> I2C1_SCL
    	  * PA10 ------> I2C1_SDA
		  */
		HAL_GPIO_DeInit(I2C_PORT, I2C_SCL_PIN | I2C_SDA_PIN);
	}
}

/**
  ***************************************************************************************************************************************
  * @brief  I2C data write
  * @param  Device address (uint16_t), memory address (uint16_t), address size (uint16_t), data (uint8_t*), data size (uint16_t)
  * @retval Status (HAL_StatusTypeDef)
  ***************************************************************************************************************************************
  */
HAL_StatusTypeDef I2C_Driver_Write(uint16_t _devAddress, uint16_t _memAddress, uint16_t _memAddrSize, uint8_t *_data, uint16_t _dataSize)
{
	return HAL_I2C_Mem_Write(&H_I2C.hal, _devAddress, _memAddress, _memAddrSize, _data, _dataSize, I2C_TIMEOUT);
}

/**
  ***************************************************************************************************************************************
  * @brief  I2C data read
  * @param  Device address (uint16_t), memory address (uint16_t), address size (uint16_t), data (uint8_t*), data size (uint16_t)
  * @retval Status (HAL_StatusTypeDef)
  ***************************************************************************************************************************************
  */
HAL_StatusTypeDef I2C_Driver_Read(uint16_t _devAddress, uint16_t _memAddress, uint16_t _memAddrSize, uint8_t *_data, uint16_t _dataSize)
{
	return HAL_I2C_Mem_Read(&H_I2C.hal, _devAddress, _memAddress, _memAddrSize, _data, _dataSize, I2C_TIMEOUT);
}
