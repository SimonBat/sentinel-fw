#ifndef __I2C_H
#define __I2C_H

#include "stm32l4xx_hal.h"

#define I2C_SCL_PIN 	GPIO_PIN_9
#define I2C_SDA_PIN 	GPIO_PIN_10
#define I2C_PORT 		GPIOA
#define I2C_TIMEOUT		10 /* ms */

typedef struct {
	I2C_HandleTypeDef hal;
	uint8_t initFlag;
} i2c_ts;

/* Global functions declarations */
void I2C_Driver_Init(void);
HAL_StatusTypeDef I2C_Driver_Write(uint16_t _devAddress, uint16_t _memAddress, uint16_t _memAddrSize, uint8_t *_data, uint16_t _dataSize);
HAL_StatusTypeDef I2C_Driver_Read(uint16_t _devAddress, uint16_t _memAddress, uint16_t _memAddrSize, uint8_t *_data, uint16_t _dataSize);

#endif
