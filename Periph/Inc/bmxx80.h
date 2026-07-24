/**
  ******************************************************************************
  * @file           : bmxx80.h
  * @brief          : Shared Bosch BMx280/BMx680 device definitions.
  ******************************************************************************
  */

#ifndef __BMXX80_H
#define __BMXX80_H

#include <stdbool.h>
#include <stdint.h>
#include "misc.h"
#include "stm32f103xb.h"

typedef struct {
  uint32_t pressure;
  int32_t temperature;
  uint16_t humidity;
  uint16_t gas_resistance;
  uint8_t gas_range;
  bool gas_valid;
  bool heater_stable;
} BMxX80_results_t;

typedef struct {
  int16_t par_t1;
  int16_t par_t2;
  int16_t par_t3;
  int16_t par_p1;
  int16_t par_p2;
  int16_t par_p3;
  int16_t par_p4;
  int16_t par_p5;
  int16_t par_p6;
  int16_t par_p7;
  int16_t par_p8;
  int16_t par_p9;
  int16_t par_p10;
  int16_t par_h1;
  int16_t par_h2;
  int16_t par_h3;
  int16_t par_h4;
  int16_t par_h5;
  int16_t par_h6;
  int16_t par_h7;
  int16_t par_g1;
  int16_t par_g2;
  int16_t par_g3;
} BMx680_calib_t;

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
} BMx280_calib_t;

typedef struct {
  uint8_t DevID;
  uint8_t* RawBufPtr;
  BMxX80_results_t Results;
  void* CalibPtr;
  FunctionalState Lock;
  I2C_TypeDef* I2Cx;
  uint8_t I2C_Address;
} BMxX80_TypeDef;

BMxX80_TypeDef* Get_BoschDevice(uint16_t);

#endif /* __BMXX80_H */
