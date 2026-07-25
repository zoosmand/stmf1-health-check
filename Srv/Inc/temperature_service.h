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

typedef enum {
  SENSOR_HEALTH_INITIALIZING = 0U,
  SENSOR_HEALTH_HEALTHY,
  SENSOR_HEALTH_DEGRADED,
  SENSOR_HEALTH_FAILED,
  SENSOR_HEALTH_STALE,
  SENSOR_HEALTH_MISSING
} SensorHealthState_TypeDef;

typedef enum {
  SENSOR_ERROR_NONE = 0U,
  SENSOR_ERROR_NOT_READY,
  SENSOR_ERROR_TIMEOUT,
  SENSOR_ERROR_CRC,
  SENSOR_ERROR_BUS,
  SENSOR_ERROR_MISSING,
  SENSOR_ERROR_CONVERSION
} SensorError_TypeDef;

typedef struct {
  SensorModel_TypeDef model;
  uint8_t capabilities;
  uint8_t identity[8];
  uint32_t serialNumber;
  BaseType_t dataValid;
  int32_t temperature;
  uint32_t pressure;
  uint32_t humidity;
  TickType_t lastAttempt;
  TickType_t lastSuccess;
  uint16_t consecutiveFailures;
  SensorHealthState_TypeDef health;
  SensorError_TypeDef lastError;
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
