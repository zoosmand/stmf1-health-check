/**
  ******************************************************************************
  * @file           : bmx280.h
  * @brief          : Bosch BMP280/BME280 sensor interface.
  ******************************************************************************
  */

#ifndef __BMX280_H
#define __BMX280_H

#include "bmxx80.h"

#define BMX280_MODEL                 280U
#define BMX280_I2C_ADDR              0x76U
#define BMP280_ID                    0x58U
#define BME280_ID                    0x60U

#define BMX280_DEV_ID                0xD0U
#define BMX280_CALIB1                0x88U
#define BMX280_CALIB2                0xE1U
#define BMX280_CTRL_HUM              0xF2U
#define BMX280_STATUS                0xF3U
#define BMX280_CTRL_MEAS             0xF4U
#define BMX280_SETTINGS              0xF5U
#define BMX280_DATA                  0xF7U

#define BMX280_MEASURING             0x08U
#define BMX280_TEMPERATURE_OVS_X4    (0x03U << 5U)
#define BMX280_PRESSURE_OVS_X4       (0x03U << 2U)
#define BMX280_HUMIDITY_OVS_X4       0x03U
#define BMX280_CONFIG_INACTIVE_250   (0x03U << 5U)
#define BMX280_CONFIG_FILTER_4       (0x02U << 2U)
#define BMX280_FORCE_MODE            0x01U

ErrorStatus BMx280_Init(BMxX80_TypeDef*);
ErrorStatus BMx280_Measurement(BMxX80_TypeDef*);

#endif /* __BMX280_H */
