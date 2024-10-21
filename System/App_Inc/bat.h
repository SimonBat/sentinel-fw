#ifndef __BAT_H
#define __BAT_H

#include "bsp.h"
#include "stc3115_Driver.h"

#define BAT_UPDATE_TMO			1000U

typedef struct {
	STC3115_ConfigData_TypeDef config;
	STC3115_BatteryData_TypeDef data;
	__IO uint16_t updateTmo;
	__IO uint16_t offTmo;
	uint8_t initFlag;
} bat_ts;

/* Global functions declarations */
void BAT_Init(void);
void BAT_Handler(void);
uint8_t BAT_Get_SOC(void);
void BAT_Update_TMO(void);

#endif
