/**
  ******************************************************************************
  * @file           : ssd13xx.h
  * @brief          : SSD1306 and SSD1315 display interface over I2C.
  * @project        : STM32F1 Health Check Device
  * @platform       : STMicroelectronics STM32F103C8
  * @created        : 25.07.2026 03:58:46 PM
  ******************************************************************************
  * @attention
  * @copyright  : 2017-2026, Dmitry Slobodchikov
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

/**
  * @brief Fonts supported by the SSD13xx text renderer.
  */
typedef enum {
  SSD13XX_FONT_5X7 = 57U,
  SSD13XX_FONT_10X14 = 1014U
} SSD13xx_Font_TypeDef;

/**
  * @brief Runtime configuration and output state for one SSD13xx display.
  * @param i2c (I2C_TypeDef*) I2C peripheral connected to the display.
  * @param address (uint8_t) Seven-bit I2C address.
  * @param model (uint16_t) SSD controller model number.
  * @param font (SSD13xx_Font_TypeDef) Active text font.
  */
typedef struct {
  I2C_TypeDef* i2c;
  uint8_t address;
  uint16_t model;
  SSD13xx_Font_TypeDef font;
} SSD13xx_TypeDef;

/**
  * @brief Initialize an SSD1306 or SSD1315 display.
  * @param display (SSD13xx_TypeDef*) State object to initialize.
  * @param i2c (I2C_TypeDef*) I2C peripheral connected to the display.
  * @param address (uint8_t) Seven-bit I2C address.
  * @param model (uint16_t) Supported SSD controller model number.
  * @param font (SSD13xx_Font_TypeDef) Initial text font.
  * @retval (ErrorStatus) SUCCESS when initialization completes.
  */
ErrorStatus SSD13xx_Init(
  SSD13xx_TypeDef* display,
  I2C_TypeDef* i2c,
  uint8_t address,
  uint16_t model,
  SSD13xx_Font_TypeDef font
);

/**
  * @brief Clear the display framebuffer and reset the text cursor.
  * @param display (SSD13xx_TypeDef*) Initialized display state.
  * @retval (ErrorStatus) SUCCESS when the display accepts the update.
  */
ErrorStatus SSD13xx_Clear(SSD13xx_TypeDef* display);

/**
  * @brief Append one character to the active SSD13xx text output.
  * @param character (char) Character to append.
  * @retval (int) The supplied character.
  */
int SSD13xx_PutChar(char character);

#ifdef __cplusplus
}
#endif

#endif /* __SSD13XX_H */
