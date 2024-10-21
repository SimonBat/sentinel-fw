#ifndef __BSP_H
#define __BSP_H

#include "stm32l4xx_hal.h"

/* Private GPIO defines */
#define BSP_BT_LCLK_PIN 			GPIO_PIN_2
#define BSP_BT_LCLK_PORT 			GPIOA
#define BSP_FLASH_CLK_PIN 			GPIO_PIN_3
#define BSP_FLASH_CLK_PORT 			GPIOA
#define BSP_FLASH_IO3_PIN 			GPIO_PIN_6
#define BSP_FLASH_IO3_PORT 			GPIOA
#define BSP_FLASH_IO2_PIN 			GPIO_PIN_7
#define BSP_FLASH_IO2_PORT 			GPIOA
#define BSP_FLASH_IO1_PIN 			GPIO_PIN_0
#define BSP_FLASH_IO1_PORT 			GPIOB
#define BSP_FLASH_IO0_PIN 			GPIO_PIN_1
#define BSP_FLASH_IO0_PORT 			GPIOB
#define BSP_SYS_OFF_PIN 			GPIO_PIN_10
#define BSP_SYS_OFF_PORT 			GPIOB
#define BSP_FLASH_NCS_PIN 			GPIO_PIN_11
#define BSP_FLASH_NCS_PORT 			GPIOB
#define BSP_USB_VBUS_PIN 			GPIO_PIN_8
#define BSP_USB_VBUS_PORT 			GPIOA
#define BSP_USB_DM_PIN 				GPIO_PIN_11
#define BSP_USB_DP_PIN 				GPIO_PIN_12
#define BSP_USB_PORT 				GPIOA
#define BSP_TC1_IO1_PIN 			GPIO_PIN_12
#define BSP_TC1_IO2_PIN 			GPIO_PIN_13
#define BSP_TC1_IO3_PIN 			GPIO_PIN_14
#define BSP_TC1_IO4_PIN 			GPIO_PIN_15
#define BSP_TC2_IO1_PIN 			GPIO_PIN_4
#define BSP_TC2_IO2_PIN 			GPIO_PIN_5
#define BSP_TC2_IO3_PIN 			GPIO_PIN_6
#define BSP_TC_IO_PORT 				GPIOB

/* Global functions prototypes */
void BSP_System_Clock_Config(void);
void BSP_System_GPIO_Init(void);
void BSP_Error_Handler(void);
void BSP_System_off(void);

#endif
