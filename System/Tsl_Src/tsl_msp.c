/**
  ***************************************************************************************************************************************
  * @file     tsl_msp.c
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

#include "tsl_msp.h"
#include "bsp.h"

/* Global handle */
TSC_HandleTypeDef htsc;

/* Local functions definitions */
static void TSC_Peripheral_Init(void);

/**
  ***************************************************************************************************************************************
  * @brief TSC Initialization Function
  * @param None
  * @retval None
  ***************************************************************************************************************************************
  */
static void TSC_Peripheral_Init(void)
{
	/* Configure the TSC peripheral */
	htsc.Instance = TSC;
	htsc.Init.CTPulseHighLength = TSC_CTPH_2CYCLES;
	htsc.Init.CTPulseLowLength = TSC_CTPL_2CYCLES;
	htsc.Init.SpreadSpectrum = DISABLE;
	htsc.Init.SpreadSpectrumDeviation = 1U;
	htsc.Init.SpreadSpectrumPrescaler = TSC_SS_PRESC_DIV1;
	htsc.Init.PulseGeneratorPrescaler = TSC_PG_PRESC_DIV4;
	htsc.Init.MaxCountValue = TSC_MCV_8191;
	htsc.Init.IODefaultMode = TSC_IODEF_OUT_PP_LOW;
	htsc.Init.SynchroPinPolarity = TSC_SYNC_POLARITY_FALLING;
	htsc.Init.AcquisitionMode = TSC_ACQ_MODE_NORMAL;
	htsc.Init.MaxCountInterrupt = DISABLE;
	htsc.Init.ChannelIOs = TSC_GROUP1_IO2 | TSC_GROUP1_IO3 | TSC_GROUP1_IO4 | TSC_GROUP2_IO2 | TSC_GROUP2_IO3;
	htsc.Init.ShieldIOs = 0U;
	htsc.Init.SamplingIOs = TSC_GROUP1_IO1 | TSC_GROUP2_IO1;
	if(HAL_OK != HAL_TSC_Init(&htsc)){BSP_Error_Handler();}
}

/**
  ***************************************************************************************************************************************
  * @brief TSC MSP Initialization
  * This function configures the hardware resources used in this example
  * @param htsc: TSC handle (TSC_HandleTypeDef*)
  * @retval None
  ***************************************************************************************************************************************
  */
void HAL_TSC_MspInit(TSC_HandleTypeDef* htsc)
{
	GPIO_InitTypeDef GPIO_InitStruct={0};

	if(TSC == htsc->Instance)
	{
		/* Peripheral clock enable */
		__HAL_RCC_TSC_CLK_ENABLE();
		__HAL_RCC_GPIOB_CLK_ENABLE();

		/** TSC GPIO Configuration
    	  * PB12 ------> TSC_G1_IO1
    	  * PB13 ------> TSC_G1_IO2
    	  * PB14 ------> TSC_G1_IO3
    	  * PB15 ------> TSC_G1_IO4
    	  * PB4 ------> TSC_G2_IO1
    	  * PB5 ------> TSC_G2_IO2
    	  * PB6 ------> TSC_G2_IO3
    	  */
		GPIO_InitStruct.Pin = BSP_TC1_IO1_PIN | BSP_TC2_IO1_PIN;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		GPIO_InitStruct.Alternate = GPIO_AF9_TSC;
		HAL_GPIO_Init(BSP_TC_IO_PORT, &GPIO_InitStruct);

		GPIO_InitStruct.Pin = BSP_TC1_IO2_PIN | BSP_TC1_IO3_PIN | BSP_TC1_IO4_PIN | BSP_TC2_IO2_PIN | BSP_TC2_IO3_PIN;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		GPIO_InitStruct.Alternate = GPIO_AF9_TSC;
		HAL_GPIO_Init(BSP_TC_IO_PORT, &GPIO_InitStruct);
	}
}

/**
  ***************************************************************************************************************************************
  * @brief TSC MSP De-Initialization
  * This function freeze the hardware resources used in this example
  * @param htsc: TSC handle (TSC_HandleTypeDef*)
  * @retval None
  ***************************************************************************************************************************************
  */
void HAL_TSC_MspDeInit(TSC_HandleTypeDef* htsc)
{
	if(TSC == htsc->Instance)
	{
		/* Peripheral clock disable */
		__HAL_RCC_TSC_CLK_DISABLE();

		/** TSC GPIO Configuration
		  * PB12 ------> TSC_G1_IO1
		  * PB13 ------> TSC_G1_IO2
		  * PB14 ------> TSC_G1_IO3
		  * PB15 ------> TSC_G1_IO4
		  * PB4 ------> TSC_G2_IO1
		  * PB5 ------> TSC_G2_IO2
		  * PB6 ------> TSC_G2_IO3
		  */
		HAL_GPIO_DeInit(BSP_TC_IO_PORT, BSP_TC1_IO1_PIN | BSP_TC1_IO2_PIN | \
						BSP_TC1_IO3_PIN | BSP_TC1_IO4_PIN | BSP_TC2_IO1_PIN | BSP_TC2_IO2_PIN | BSP_TC2_IO3_PIN);
	}
}

/**
  ***************************************************************************************************************************************
  * @brief TSL initialization function
  * @param None
  * @retval None
  ***************************************************************************************************************************************
  */
void TSL_Driver_Init(void)
{
	TSC_Peripheral_Init();
	TSL_User_Init();
}
