/**
  ******************************************************************************
  * @file           : ssd13xx.h
  * @brief          : SSD1306/SSD1315 display interface over I2C.
  ******************************************************************************
  */

#ifndef __SSD13XX_H
#define __SSD13XX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#define SSD1306_MODEL 1306U
#define SSD1315_MODEL 1315U

#define SSD13XX_I2C_ADDR 0x3CU
#define SSD13XX_WIDTH    128U
#define SSD13XX_HEIGHT   64U

typedef enum {
  SSD13XX_FONT_5X7 = 57U,
  SSD13XX_FONT_10X14 = 1014U
} SSD13xx_Font_TypeDef;

typedef struct {
  I2C_TypeDef* I2C;
  uint8_t Address;
  uint16_t Model;
  SSD13xx_Font_TypeDef Font;
} SSD13xx_TypeDef;

ErrorStatus SSD13xx_Init(
  SSD13xx_TypeDef*,
  I2C_TypeDef*,
  uint8_t,
  uint16_t,
  SSD13xx_Font_TypeDef
);
ErrorStatus SSD13xx_Clear(SSD13xx_TypeDef*);
int putc_dspl_ssd(char);

#ifdef __cplusplus
}
#endif

#endif /* __SSD13XX_H */
