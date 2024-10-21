#ifndef __HCITRCFG_H_
#define __HCITRCFG_H_

#include "BTAPITyp.h"
#include "stm32l4xx_ll_rcc.h"
#include "stm32l4xx_ll_bus.h"
#include "stm32l4xx_ll_system.h"
#include "stm32l4xx_ll_exti.h"
#include "stm32l4xx_ll_cortex.h"
#include "stm32l4xx_ll_utils.h"
#include "stm32l4xx_ll_pwr.h"
#include "stm32l4xx_ll_usart.h"
#include "stm32l4xx_ll_gpio.h"

#define HCITR_UART              	4
#define HCITR_UART_AF				8

#define HCITR_PORT_1           		A
#define HCITR_PORT_1_AHB			2
#define HCITR_P1_TXD_PIN       	 	0
#define HCITR_P1_RXD_PIN        	1
#define HCITR_P1_RTS_PIN        	15

#define HCITR_PORT_2           		B
#define HCITR_PORT_2_AHB			2
#define HCITR_P2_CTS_PIN        	7
#define HCITR_P2_RESET_PIN      	8

/** Define the following to enable debug logging of HCI traffic.
  * If this macro is defined, all incoming and outgoing traffic will be
  * logged via BTPS_OutputMessage()
  */
/* #define HCITR_ENABLE_DEBUG_LOGGING */

/* Standard C style concatenation macros */
#define DEF_CONCAT2(_x_, _y_)			__DEF_CONCAT2__(_x_, _y_)
#define __DEF_CONCAT2__(_x_, _y_)   	_x_ ## _y_

#define DEF_CONCAT3(_x_, _y_, _z_)     	__DEF_CONCAT3__(_x_, _y_, _z_)
#define __DEF_CONCAT3__(_x_, _y_, _z_) 	_x_ ## _y_ ## _z_

/* Determine the Peripheral bus that is used by the UART */
#if ((HCITR_UART == 1)||(HCITR_UART == 6))
   #define HCITR_UART_APB			2
#else
   #define HCITR_UART_APB         	1
#endif

/* Determine the type of UART */
#if((HCITR_UART == 1)||(HCITR_UART == 2)||(HCITR_UART == 3)||(HCITR_UART == 6))
   #define HCITR_UART_TYPE			USART
#elif(HCITR_UART == 4)
   #define HCITR_UART_TYPE			UART
#endif

/* UART control mapping */
#define HCITR_UART_BASE                		(DEF_CONCAT2(HCITR_UART_TYPE, HCITR_UART))
#define HCITR_UART_IRQ                 		(DEF_CONCAT3(HCITR_UART_TYPE, HCITR_UART, _IRQn))
#define HCITR_UART_IRQ_HANDLER         		(DEF_CONCAT3(HCITR_UART_TYPE, HCITR_UART, _IRQHandler))

#define HCITR_RCC_CLK_CMD_ENABLE_UART  		(DEF_CONCAT3(DEF_CONCAT2(LL_APB,HCITR_UART_APB),DEF_CONCAT2(_GRP,HCITR_UART_APB), _EnableClock))
#define HCITR_RCC_CLK_CMD_DISABLE_UART  	(DEF_CONCAT3(DEF_CONCAT2(LL_APB,HCITR_UART_APB),DEF_CONCAT2(_GRP,HCITR_UART_APB), _DisableClock))
#define HCITR_RCC_CLK_UART  		 		(DEF_CONCAT3(DEF_CONCAT2(LL_APB,HCITR_UART_APB),DEF_CONCAT2(_GRP,HCITR_UART_APB), DEF_CONCAT2(_PERIPH_UART,HCITR_UART)))
#define HCITR_RCC_CLK_CMD_ENABLE_GPIO_1  	(DEF_CONCAT3(DEF_CONCAT2(LL_AHB,HCITR_PORT_1_AHB),_GRP1, _EnableClock))
#define HCITR_RCC_CLK_GPIO_1  		 		(DEF_CONCAT3(DEF_CONCAT2(LL_AHB,HCITR_PORT_1_AHB),_GRP1, DEF_CONCAT2(_PERIPH_GPIO,HCITR_PORT_1)))
#define HCITR_RCC_CLK_CMD_ENABLE_GPIO_2  	(DEF_CONCAT3(DEF_CONCAT2(LL_AHB,HCITR_PORT_2_AHB),_GRP1, _EnableClock))
#define HCITR_RCC_CLK_GPIO_2  		 		(DEF_CONCAT3(DEF_CONCAT2(LL_AHB,HCITR_PORT_2_AHB),_GRP1, DEF_CONCAT2(_PERIPH_GPIO,HCITR_PORT_2)))
#define HCITR_UART_GPIO_AF             		(DEF_CONCAT2(LL_GPIO_AF_, HCITR_UART_AF))

/* GPIO mapping */
#define HCITR_GPIO_PORT_1            		(DEF_CONCAT2(GPIO, HCITR_PORT_1))
#define HCITR_GPIO_PORT_2            		(DEF_CONCAT2(GPIO, HCITR_PORT_2))

#endif
