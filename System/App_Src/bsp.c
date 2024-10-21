/**
  ***************************************************************************************************************************************
  * @file     bsp.c
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

#include "bsp.h"

/**
  ***************************************************************************************************************************************
  * @brief  System clock configuration
  * @param  None
  * @retval None
  ***************************************************************************************************************************************
  */
void BSP_System_Clock_Config(void)
{
	PWR_PVDTypeDef _sConfigPVD = { 0 };
	RCC_OscInitTypeDef _rccOscInitStruct = { 0 };
	RCC_ClkInitTypeDef _rccClkInitStruct = { 0 };
	RCC_PeriphCLKInitTypeDef _periphClkInit = { 0 };

	__HAL_RCC_SYSCFG_CLK_ENABLE();
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_RCC_RTC_ENABLE();

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOH_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/* PVD Configuration */
	_sConfigPVD.PVDLevel = PWR_PVDLEVEL_0;
	_sConfigPVD.Mode = PWR_PVD_MODE_NORMAL;
	HAL_PWR_ConfigPVD(&_sConfigPVD);
	/* Enable the PVD Output */
	HAL_PWR_EnablePVD();

	/* Configure LSE Drive Capability */
	HAL_PWR_EnableBkUpAccess();
	__HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_HIGH);
	/** Initializes the RCC Oscillators according to the specified parameters
	  * in the RCC_OscInitTypeDef structure.
	  */
	_rccOscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48 | RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_LSE;
	_rccOscInitStruct.LSEState = RCC_LSE_ON;
	_rccOscInitStruct.HSIState = RCC_HSI_ON;
	_rccOscInitStruct.HSI48State = RCC_HSI48_ON;
	_rccOscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	_rccOscInitStruct.PLL.PLLState = RCC_PLL_ON;
	_rccOscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	_rccOscInitStruct.PLL.PLLM = 1U;
	_rccOscInitStruct.PLL.PLLN = 20U;
	_rccOscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
	_rccOscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV4;
	_rccOscInitStruct.PLL.PLLR = RCC_PLLR_DIV4;
	if(HAL_OK != HAL_RCC_OscConfig(&_rccOscInitStruct)){BSP_Error_Handler();}

	/* Initializes the CPU, AHB and APB buses clocks */
	_rccClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	_rccClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	_rccClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	_rccClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	_rccClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
	if(HAL_OK != HAL_RCC_ClockConfig(&_rccClkInitStruct, FLASH_LATENCY_4)){BSP_Error_Handler();}

	_periphClkInit.PeriphClockSelection = RCC_PERIPHCLK_UART4 | RCC_PERIPHCLK_I2C1 | RCC_PERIPHCLK_USB | RCC_PERIPHCLK_RTC;
	_periphClkInit.Uart4ClockSelection = RCC_UART4CLKSOURCE_SYSCLK;
	_periphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_SYSCLK;
	_periphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;
	_periphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
	if(HAL_OK != HAL_RCCEx_PeriphCLKConfig(&_periphClkInit)){BSP_Error_Handler();}
	HAL_RCCEx_EnableLSCO(RCC_LSCOSOURCE_LSE);

	/* Configure the main internal regulator output voltage */
	if(HAL_OK != HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1)){BSP_Error_Handler();}
}

/**
  ***************************************************************************************************************************************
  * @brief  GPIO initialization function
  * @param  None
  * @retval None
  ***************************************************************************************************************************************
  */
void BSP_System_GPIO_Init(void)
{
	GPIO_InitTypeDef _gpioInitStruct = { 0 };

	/* Configure GPIO pins : LED_Pin SYS_OFF_Pin BT_NSHUTD_Pin */
	_gpioInitStruct.Pin = BSP_SYS_OFF_PIN;
	_gpioInitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	_gpioInitStruct.Pull = GPIO_NOPULL;
	_gpioInitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	/* Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(BSP_SYS_OFF_PORT, BSP_SYS_OFF_PIN, GPIO_PIN_SET);
	HAL_GPIO_Init(BSP_SYS_OFF_PORT, &_gpioInitStruct);

	/* Configure unused GPIO pin : PC13 */
	_gpioInitStruct.Mode = GPIO_MODE_ANALOG;
	_gpioInitStruct.Pull = GPIO_NOPULL;
	_gpioInitStruct.Pin = GPIO_PIN_13;
	HAL_GPIO_Init(GPIOC, &_gpioInitStruct);
	_gpioInitStruct.Pin = BSP_USB_VBUS_PIN;
	HAL_GPIO_Init(BSP_USB_VBUS_PORT, &_gpioInitStruct);
	/* Configure unused GPIO pins : PH0 PH1 PH3 */
	_gpioInitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_3;
	HAL_GPIO_Init(GPIOH, &_gpioInitStruct);
	/* Configure unused GPIO pins : PA4 PA5 */
	_gpioInitStruct.Pin = GPIO_PIN_4 | GPIO_PIN_5;
	HAL_GPIO_Init(GPIOA, &_gpioInitStruct);
	/* Configure unused GPIO pin : PB9 */
	_gpioInitStruct.Pin = GPIO_PIN_9;
	HAL_GPIO_Init(GPIOB, &_gpioInitStruct);

	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_7, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_8, GPIO_PIN_RESET);
	_gpioInitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	_gpioInitStruct.Pull = GPIO_NOPULL;
	_gpioInitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	_gpioInitStruct.Pin = GPIO_PIN_7 | GPIO_PIN_8;
	/* Configure GPIO pin Output Level */
	HAL_GPIO_Init(GPIOB, &_gpioInitStruct);

	HAL_GPIO_WritePin(GPIOA, (GPIO_PIN_0 | GPIO_PIN_1), GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET);
	_gpioInitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	_gpioInitStruct.Pull = GPIO_NOPULL;
	_gpioInitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	_gpioInitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_15;
	/* Configure GPIO pin Output Level */
	HAL_GPIO_Init(GPIOA, &_gpioInitStruct);

	HAL_Delay(10U);

	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
	_gpioInitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	_gpioInitStruct.Pull = GPIO_NOPULL;
	_gpioInitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	_gpioInitStruct.Pin = GPIO_PIN_8;
	/* Configure GPIO pin Output Level */
	HAL_GPIO_Init(GPIOB, &_gpioInitStruct);

	_gpioInitStruct.Mode = GPIO_MODE_ANALOG;
	_gpioInitStruct.Pull = GPIO_NOPULL;
	_gpioInitStruct.Pin = GPIO_PIN_7;
	HAL_GPIO_Init(GPIOB, &_gpioInitStruct);
	_gpioInitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_15;
	HAL_GPIO_Init(GPIOA, &_gpioInitStruct);
}

/**
  ***************************************************************************************************************************************
  * @brief  This function is executed in case of error occurrence
  * @param  None
  * @retval None
  ***************************************************************************************************************************************
  */
void BSP_Error_Handler(void)
{
	/* Something wrong - system power off */
	while(1U)
	{
		HAL_GPIO_WritePin(BSP_SYS_OFF_PORT, BSP_SYS_OFF_PIN, GPIO_PIN_RESET);
		HAL_Delay(100U);
	}
}

/**
  ***************************************************************************************************************************************
  * @brief  System off handler
  * @param  None
  * @retval None
  ***************************************************************************************************************************************
  */
void BSP_System_off(void)
{
	for(uint8_t _idx = 0U; _idx < 3U; _idx++)
	{
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET);
		HAL_Delay(50U);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
		HAL_Delay(100U);
	}

	BSP_Error_Handler();
}

#ifdef  USE_FULL_ASSERT
/**
  ***************************************************************************************************************************************
  * @brief  Reports the name of the source file and the source line number where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  ***************************************************************************************************************************************
  */
void BSP_Assert_Failed(char *_file, uint32_t _line)
{
	UNUSED(_file);
	UNUSED(_line);
}
#endif
