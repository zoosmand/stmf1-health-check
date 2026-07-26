/**
  ******************************************************************************
  * @file           : bmxx80.c
  * @brief          : Bosch BMx sensor device instances.
  * @project        : STM32F1 Health Check Device
  * @platform       : STMicroelectronics STM32F103C8
  * @created        : 24.07.2026 10:05:16 PM
  ******************************************************************************
  * @attention
  * @copyright  : 2017-2026, Dmitry Slobodchikov
  ******************************************************************************
  */

#include "bmxx80.h"
#include "bmx280.h"
#include "bmx680.h"
#include <stddef.h>

static uint8_t bmx280RawData[32];
static BMx280_Calibration_TypeDef bmx280Calibration;
static BMxX80_TypeDef bmx280Device = {
  .deviceId = 0U,
  .rawBuffer = bmx280RawData,
  .results = {0},
  .calibration = &bmx280Calibration,
  .lock = DISABLE,
  .i2c = I2C1,
  .i2cAddress = BMX280_I2C_ADDR,
};

static uint8_t bmx680RawData[24];
static BMx680_Calibration_TypeDef bmx680Calibration;
static BMxX80_TypeDef bmx680Device = {
  .deviceId = 0U,
  .rawBuffer = bmx680RawData,
  .results = {0},
  .calibration = &bmx680Calibration,
  .lock = DISABLE,
  .i2c = I2C1,
  .i2cAddress = BMX680_I2C_ADDR,
};




// -------------------------------------------------------------
BMxX80_TypeDef* BMxX80_GetDevice(uint16_t model) {
  if (model == BMX280_MODEL) return (&bmx280Device);
  if (model == BMX680_MODEL) return (&bmx680Device);
  return (NULL);
}
