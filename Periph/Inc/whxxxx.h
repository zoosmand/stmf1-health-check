/**
  ******************************************************************************
  * @file           : whxxxx.h
  * @brief          : WHxxxx character display interface over I2C.
  * @project        : STM32F1 Health Check Device
  * @platform       : STMicroelectronics STM32F103C8
  * @created        : 24.07.2026 04:51:32 PM
  ******************************************************************************
  * @attention
  * @copyright  : 2017-2026, Dmitry Slobodchikov
  ******************************************************************************
  */

#ifndef __WHXXXX_H
#define __WHXXXX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#define WHXXXX_I2C_ADDRESS 0x27U

ErrorStatus WHxxxx_Init(I2C_TypeDef*, uint8_t);
ErrorStatus WHxxxx_Clear(void);
ErrorStatus WHxxxx_Print(const uint8_t*, uint16_t);
int WHxxxx_PutChar(char);

#ifdef __cplusplus
}
#endif

#endif /* __WHXXXX_H */
