/**
  ******************************************************************************
  * @file           : bmxx80.h
  * @brief          : Shared Bosch BMx280 and BMx680 device definitions.
  * @project        : STM32F1 Health Check Device
  * @platform       : STMicroelectronics STM32F103C8
  * @created        : 24.07.2026 10:05:16 PM
  ******************************************************************************
  * @attention
  * @copyright  : 2017-2026, Dmitry Slobodchikov
  ******************************************************************************
  */

#ifndef __BMXX80_H
#define __BMXX80_H

#include <stdbool.h>
#include <stdint.h>
#include "misc.h"
#include "stm32f103xb.h"

#define BMXX80_UNIQUE_ID_REG 0x83U

/**
  * @brief Compensated environmental measurements shared by Bosch sensors.
  * @param pressure (uint32_t) Pressure in pascals.
  * @param temperature (int32_t) Temperature in hundredths of a degree Celsius.
  * @param humidity (uint32_t) Relative humidity in thousandths of percent.
  * @param gasResistance (uint16_t) Raw gas-resistance result.
  * @param gasRange (uint8_t) Gas-resistance conversion range.
  * @param gasValid (bool) Whether the gas result is valid.
  * @param heaterStable (bool) Whether the gas heater reached its target.
  */
typedef struct {
  uint32_t pressure;
  int32_t temperature;
  /* Relative humidity in thousandths of percent (100000 = 100.000 %RH). */
  uint32_t humidity;
  uint16_t gasResistance;
  uint8_t gasRange;
  bool gasValid;
  bool heaterStable;
} BMxX80_Results_TypeDef;

/**
  * @brief Factory compensation coefficients read from a BME680.
  * @param par_t1 (uint16_t) Temperature coefficient T1.
  * @param par_t2 (int16_t) Temperature coefficient T2.
  * @param par_t3 (int8_t) Temperature coefficient T3.
  * @param par_p1 (uint16_t) Pressure coefficient P1.
  * @param par_p2 (int16_t) Pressure coefficient P2.
  * @param par_p3 (int8_t) Pressure coefficient P3.
  * @param par_p4 (int16_t) Pressure coefficient P4.
  * @param par_p5 (int16_t) Pressure coefficient P5.
  * @param par_p6 (int8_t) Pressure coefficient P6.
  * @param par_p7 (int8_t) Pressure coefficient P7.
  * @param par_p8 (int16_t) Pressure coefficient P8.
  * @param par_p9 (int16_t) Pressure coefficient P9.
  * @param par_p10 (uint8_t) Pressure coefficient P10.
  * @param par_h1 (uint16_t) Humidity coefficient H1.
  * @param par_h2 (uint16_t) Humidity coefficient H2.
  * @param par_h3 (int8_t) Humidity coefficient H3.
  * @param par_h4 (int8_t) Humidity coefficient H4.
  * @param par_h5 (int8_t) Humidity coefficient H5.
  * @param par_h6 (uint8_t) Humidity coefficient H6.
  * @param par_h7 (int8_t) Humidity coefficient H7.
  * @param par_g1 (int8_t) Gas coefficient G1.
  * @param par_g2 (int16_t) Gas coefficient G2.
  * @param par_g3 (int8_t) Gas coefficient G3.
  *
  * Member names follow the Bosch datasheet so each coefficient can be traced
  * directly to the compensation formulas.
  */
typedef struct {
  uint16_t par_t1;
  int16_t par_t2;
  int8_t par_t3;
  uint16_t par_p1;
  int16_t par_p2;
  int8_t par_p3;
  int16_t par_p4;
  int16_t par_p5;
  int8_t par_p6;
  int8_t par_p7;
  int16_t par_p8;
  int16_t par_p9;
  uint8_t par_p10;
  uint16_t par_h1;
  uint16_t par_h2;
  int8_t par_h3;
  int8_t par_h4;
  int8_t par_h5;
  uint8_t par_h6;
  int8_t par_h7;
  int8_t par_g1;
  int16_t par_g2;
  int8_t par_g3;
} BMx680_Calibration_TypeDef;

/**
  * @brief Factory compensation coefficients read from a BMP280 or BME280.
  * @param dig_t1 (uint16_t) Temperature coefficient T1.
  * @param dig_t2 (int16_t) Temperature coefficient T2.
  * @param dig_t3 (int16_t) Temperature coefficient T3.
  * @param dig_p1 (uint16_t) Pressure coefficient P1.
  * @param dig_p2 (int16_t) Pressure coefficient P2.
  * @param dig_p3 (int16_t) Pressure coefficient P3.
  * @param dig_p4 (int16_t) Pressure coefficient P4.
  * @param dig_p5 (int16_t) Pressure coefficient P5.
  * @param dig_p6 (int16_t) Pressure coefficient P6.
  * @param dig_p7 (int16_t) Pressure coefficient P7.
  * @param dig_p8 (int16_t) Pressure coefficient P8.
  * @param dig_p9 (int16_t) Pressure coefficient P9.
  * @param dig_h1 (uint8_t) Humidity coefficient H1.
  * @param dig_h2 (int16_t) Humidity coefficient H2.
  * @param dig_h3 (uint8_t) Humidity coefficient H3.
  * @param dig_h4 (int16_t) Humidity coefficient H4.
  * @param dig_h5 (int16_t) Humidity coefficient H5.
  * @param dig_h6 (int8_t) Humidity coefficient H6.
  *
  * Member names follow the Bosch datasheet so each coefficient can be traced
  * directly to the compensation formulas.
  */
typedef struct {
  uint16_t dig_t1;
  int16_t dig_t2;
  int16_t dig_t3;
  uint16_t dig_p1;
  int16_t dig_p2;
  int16_t dig_p3;
  int16_t dig_p4;
  int16_t dig_p5;
  int16_t dig_p6;
  int16_t dig_p7;
  int16_t dig_p8;
  int16_t dig_p9;
  uint8_t dig_h1;
  int16_t dig_h2;
  uint8_t dig_h3;
  int16_t dig_h4;
  int16_t dig_h5;
  int8_t dig_h6;
} BMx280_Calibration_TypeDef;

/**
  * @brief Runtime state shared by the BMx280 and BME680 drivers.
  * @param deviceId (uint8_t) Chip identifier read from the device.
  * @param uniqueId (uint32_t) Device value read from the unique-ID register.
  * @param rawBuffer (uint8_t*) Driver-owned raw measurement buffer.
  * @param results (BMxX80_Results_TypeDef) Latest compensated measurements.
  * @param calibration (void*) Model-specific factory calibration coefficients.
  * @param lock (FunctionalState) Driver lock state.
  * @param i2c (I2C_TypeDef*) I2C peripheral used by the device.
  * @param i2cAddress (uint8_t) Seven-bit I2C address.
  */
typedef struct {
  uint8_t deviceId;
  uint32_t uniqueId;
  uint8_t* rawBuffer;
  BMxX80_Results_TypeDef results;
  void* calibration;
  FunctionalState lock;
  I2C_TypeDef* i2c;
  uint8_t i2cAddress;
} BMxX80_TypeDef;

/**
  * @brief Return the statically allocated Bosch device for a model number.
  * @param model (uint16_t) Bosch model selector, such as 280 or 680.
  * @retval (BMxX80_TypeDef*) Device state, or NULL for an unsupported model.
  */
BMxX80_TypeDef* BMxX80_GetDevice(uint16_t);

#endif /* __BMXX80_H */
