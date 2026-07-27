/**
  ******************************************************************************
  * @file           : fonts.h
  * @brief          : Bitmap font definitions for graphical displays.
  * @project        : STM32F1 Health Check Device
  * @platform       : STMicroelectronics STM32F103C8
  * @created        : 25.07.2026 03:58:46 PM
  ******************************************************************************
  * @attention
  * @copyright  : 2017-2026, Dmitry Slobodchikov
  ******************************************************************************
  */

#ifndef __FONTS_H
#define __FONTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define FONT_FIRST_CHARACTER 32U
#define FONT_LAST_CHARACTER  126U
#define FONT_DEGREE_CHARACTER 176U
#define FONT_CHARACTER_COUNT 96U

typedef uint8_t font_dot_5x7_t[6];
typedef uint8_t font_dot_10x14_t[24];

extern const font_dot_5x7_t font_dot_5x7[FONT_CHARACTER_COUNT];
extern const font_dot_10x14_t font_dot_10x14[FONT_CHARACTER_COUNT];

#ifdef __cplusplus
}
#endif

#endif /* __FONTS_H */
