/**
  ******************************************************************************
  * @file           : bmx680.h
  * @brief          : Bosch BME680 sensor interface.
  ******************************************************************************
  */

#ifndef __BMX680_H
#define __BMX680_H

#include "bmxx80.h"

#define BMX680_MODEL                 680U
#define BMX680_I2C_ADDR              0x77U
#define BME680_ID                    0x61U

#define BMX680_DEV_ID                0xD0U
#define BMX680_CALIB1                0x8AU
#define BMX680_CALIB2                0xE1U
#define BMX680_CTRL_HUM              0x72U
#define BMX680_CTRL_MEAS             0x74U
#define BMX680_STATUS                0x1DU
#define BMX680_TEMP_MSB              0x22U

#define BMX680_MEASURING             (1U << 5U)
#define BMX680_HUMIDITY_OVS_0        0x00U
#define BMX680_PRESSURE_OVS_0        (0x00U << 2U)
#define BMX680_TEMPERATURE_OVS_8     (0x04U << 5U)
#define BMX680_FORCED_MODE           0x01U

ErrorStatus BMx680_Init(BMxX80_TypeDef*);
ErrorStatus BMx680_Measurement(BMxX80_TypeDef*);

#endif /* __BMX680_H */
