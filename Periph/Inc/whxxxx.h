/**
  ******************************************************************************
  * @file           : whxxxx.h
  * @brief          : WHxxxx character display interface over I2C.
  ******************************************************************************
  */

#ifndef __WHXXXX_H
#define __WHXXXX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#define WHxxxx_I2C_ADDR 0x27U

ErrorStatus WHxxxx_Init(I2C_TypeDef*, uint8_t);
ErrorStatus WHxxxx_Clear(void);
ErrorStatus WHxxxx_Print(const uint8_t*, uint16_t);
int putc_dspl_wh(char);

#ifdef __cplusplus
}
#endif

#endif /* __WHXXXX_H */
