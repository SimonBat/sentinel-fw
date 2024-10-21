#ifndef __DISPLAY_H
#define __DISPLAY_H

#include "stm32l4xx_hal.h"
#include "bsp.h"
#include "bat.h"

#define DISPLAY_MAX_CONTEXTS	4
#define DISPLAY_UPDATE_TMO		60 /* ms */
#define DISPLAY_PASSWORD_NBR	5

typedef struct{
	uint8_t context;
	__IO uint16_t updateTmo;
	uint8_t passwordNbr;
	uint8_t verticalListIdx;
	uint16_t horizontalListIdx;
	uint8_t xDirection;
	int16_t xScroll;
	uint8_t btFlag;
}display_ts;

/* Global functions declarations */
void DISPLAY_Prepare_Context(display_ts* _display);

#endif
