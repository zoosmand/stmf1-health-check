/**
  ******************************************************************************
  * @file           : i2c.h
  * @brief          : I2C master-mode peripheral interface.
  ******************************************************************************
  */

#ifndef __I2C_H
#define __I2C_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#define I2C1_SCL_Port GPIOB
#define I2C1_SCL_Pin  GPIO_PIN_6
#define I2C1_SDA_Port GPIOB
#define I2C1_SDA_Pin  GPIO_PIN_7

ErrorStatus I2C_Init(I2C_TypeDef*);
ErrorStatus I2C_Master_Send(I2C_TypeDef*, uint8_t, const uint8_t*, uint16_t);
ErrorStatus I2C_Master_ReadRegister(I2C_TypeDef*, uint8_t, uint8_t, uint8_t*, uint16_t);

#ifdef __cplusplus
}
#endif

#endif /* __I2C_H */
