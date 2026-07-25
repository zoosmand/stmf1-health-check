/**
  ******************************************************************************
  * @file           : temperature_service.h
  * @brief          : Periodic temperature sensor service.
  ******************************************************************************
  */

#ifndef __TEMPERATURE_SERVICE_H
#define __TEMPERATURE_SERVICE_H

#include "main.h"

#define SENSOR_SERVICE_MAX_DS18B20 6U
#define SENSOR_SERVICE_MAX_DEVICES (SENSOR_SERVICE_MAX_DS18B20 + 2U)

typedef enum {
  SENSOR_MODEL_DS18B20 = 0U,
  SENSOR_MODEL_BMP280,
  SENSOR_MODEL_BME280,
  SENSOR_MODEL_BME680
} SensorModel_TypeDef;

typedef enum {
  SENSOR_CAPABILITY_TEMPERATURE = (1U << 0U),
  SENSOR_CAPABILITY_PRESSURE    = (1U << 1U),
  SENSOR_CAPABILITY_HUMIDITY    = (1U << 2U)
} SensorCapability_TypeDef;

typedef struct {
  SensorModel_TypeDef model;
  uint8_t capabilities;
  BaseType_t dataValid;
  int32_t temperature;
  uint32_t pressure;
  uint32_t humidity;
} SensorSnapshot_TypeDef;

typedef struct {
  uint8_t temperature;
  uint8_t pressure;
  uint8_t humidity;
  uint8_t all;
} SensorCounts_TypeDef;

void TemperatureSensorService_Init(void);
void TemperatureSensorService_GetCounts(SensorCounts_TypeDef*);
ErrorStatus TemperatureSensorService_GetSnapshot(
  uint8_t,
  SensorSnapshot_TypeDef*
);
ErrorStatus TemperatureSensorService_GetByCapability(
  SensorCapability_TypeDef,
  uint8_t,
  SensorSnapshot_TypeDef*
);

#endif /* __TEMPERATURE_SERVICE_H */
