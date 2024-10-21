#ifndef __SYSTEM_H
#define __SYSTEM_H

#include "stm32l4xx_hal.h"
#include "display.h"

#define SYSTEM_BUTTONS			5
#define	SYSTEM_BUTTON_UP		1
#define SYSTEM_BUTTON_OK		4
#define SYSTEM_BUTTON_DOWN		3
#define SYSTEM_BUTTON_LEFT		0
#define SYSTEM_BUTTON_RIGHT		2
#define SYSTEM_BUTTON_ON_TMO	25
#define SYSTEM_BUTTON_OFF_TMO	25
#define SYSTEM_PASSWORD_NBR		DISPLAY_PASSWORD_NBR
#define SYSTEM_KEY_TMO			500
#define SYSTEM_OFF_TMO			300000UL

typedef struct{
	display_ts display;

	struct{
		__IO uint16_t onTmo;
		__IO uint16_t offTmo;
		uint8_t statusFlag;
	}button[SYSTEM_BUTTONS];

	uint8_t password[SYSTEM_PASSWORD_NBR];
	__IO uint8_t passwordNbr;
	__IO uint16_t passwordTmo;
	__IO uint16_t ledHandlerTmo;
	__IO uint16_t horizontalListIdxTmo;
	__IO uint16_t verticalListIdxTmo;
	__IO uint16_t dataTxTmo;
	__IO uint16_t batteryLevelTmo;
	__IO uint32_t offTmo;
}system_ts;

/* Global functions definitions */
void SYSTEM_Update_TMO(void);

#endif
