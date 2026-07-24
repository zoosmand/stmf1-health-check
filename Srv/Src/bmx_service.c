/**
  ******************************************************************************
  * @file           : bmx_service.c
  * @brief          : Bosch BMx sensor device configuration.
  ******************************************************************************
  */

#include "bmx_service.h"
#include "bmx280.h"
#include "bmx680.h"
#include <stddef.h>

static uint8_t bmx280RawData[32];
static BMx280_calib_t bmx280Calibration;
static BMxX80_TypeDef bmx280Device = {
  .DevID = 0U,
  .RawBufPtr = bmx280RawData,
  .Results = {0},
  .CalibPtr = &bmx280Calibration,
  .Lock = DISABLE,
  .I2Cx = I2C1,
  .I2C_Address = BMX280_I2C_ADDR,
};

static uint8_t bmx680RawData[24];
static BMx680_calib_t bmx680Calibration;
static BMxX80_TypeDef bmx680Device = {
  .DevID = 0U,
  .RawBufPtr = bmx680RawData,
  .Results = {0},
  .CalibPtr = &bmx680Calibration,
  .Lock = DISABLE,
  .I2Cx = I2C1,
  .I2C_Address = BMX680_I2C_ADDR,
};




// -------------------------------------------------------------
BMxX80_TypeDef* Get_BoschDevice(uint16_t model) {
  if (model == BMX280_MODEL) return (&bmx280Device);
  if (model == BMX680_MODEL) return (&bmx680Device);
  return (NULL);
}
