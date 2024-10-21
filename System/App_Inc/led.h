#ifndef __LED_H
#define __LED_H

#include "stm32l4xx_hal.h"

#define LED_PIN 			GPIO_PIN_2
#define LED_PORT 			GPIOB
#define LED_HANDLER_TMO		25 /* ms */

typedef enum {
	LED_STATE_OFF,
	LED_STATE_ON,
	LED_STATE_PULSE,
	LED_STATE_BLINK_OFF,
	LED_STATE_BLINK_ON
} led_state_te;

typedef enum {
	LED_BLINK_TMO_FAST = 250,
	LED_BLINK_TMO_SLOW = 1000
} led_blink_tmo_te;

typedef struct {
	led_state_te state;
	__IO uint32_t tmo;
	__IO uint32_t tmoSet;
} led_ts;

/* Global functions declarations */
void LED_Init(void);
void LED_Handler(void *_userParam);
void LED_On(void);
void LED_Off(void);
void LED_Pulse(void);
void LED_Blink(led_blink_tmo_te _blinkTmo);

#endif
