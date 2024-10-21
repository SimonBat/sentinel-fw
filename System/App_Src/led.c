/**
 ***************************************************************************************************************************************
 * @file     led.c
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

#include "led.h"

static led_ts HLED;

/**
  ***************************************************************************************************************************************
  * @brief  LED initialization
  * @param  None
  * @retval None
  ***************************************************************************************************************************************
  */
void LED_Init(void)
{
	GPIO_InitTypeDef _gpioInitStruct = { 0 };

	_gpioInitStruct.Pin = LED_PIN;
	_gpioInitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	_gpioInitStruct.Pull = GPIO_NOPULL;
	_gpioInitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
	HAL_GPIO_Init(LED_PORT, &_gpioInitStruct);
	HLED.state = LED_STATE_OFF;
}

/**
  ***************************************************************************************************************************************
  * @brief LED handler task
  * @param User parameter (void*)
  * @retval None
  ***************************************************************************************************************************************
  */
void LED_Handler(void *_userParam)
{
	if(HLED.tmo > LED_HANDLER_TMO){HLED.tmo -= LED_HANDLER_TMO;}
	else{HLED.tmo = 0U;}

	switch(HLED.state)
	{
		case(LED_STATE_OFF):
		case(LED_STATE_ON): break;

		case(LED_STATE_PULSE):
			if(!HLED.tmo){LED_Off();}
		break;

		case(LED_STATE_BLINK_ON):
			if(!HLED.tmo)
			{
				HLED.tmo = HLED.tmoSet;
				HLED.state = LED_STATE_BLINK_OFF;
				HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
			}
		break;

		case(LED_STATE_BLINK_OFF):
			if(!HLED.tmo)
			{
				HLED.state = LED_STATE_BLINK_ON;
				HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
			}
		break;
	}
}

/**
  ***************************************************************************************************************************************
  * @brief  LED on
  * @param  None
  * @retval None
  ***************************************************************************************************************************************
  */
void LED_On(void)
{
	HLED.tmo = 0U;
	HLED.state = LED_STATE_ON;
	HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
}

/**
  ***************************************************************************************************************************************
  * @brief  LED off
  * @param  None
  * @retval None
  ***************************************************************************************************************************************
  */
void LED_Off(void)
{
	HLED.tmo = 0U;
	HLED.state = LED_STATE_OFF;
	HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
}

/**
  ***************************************************************************************************************************************
  * @brief  LED pulse
  * @param  None
  * @retval None
  ***************************************************************************************************************************************
  */
void LED_Pulse(void)
{
	HLED.tmo = LED_HANDLER_TMO * 2U;
	HLED.state = LED_STATE_PULSE;
	HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
}

/**
  ***************************************************************************************************************************************
  * @brief  LED blink
  * @param  Blink tmo (led_blink_tmo_te)
  * @retval None
  ***************************************************************************************************************************************
  */
void LED_Blink(led_blink_tmo_te _blinkTmo)
{
	HLED.tmo = LED_HANDLER_TMO * 3U;
	HLED.tmoSet = _blinkTmo;
	HLED.state = LED_STATE_BLINK_ON;
	HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
}
