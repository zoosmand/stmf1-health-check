/**
  ******************************************************************************
  * @file           : temperature_service.h
  * @brief          : Periodic temperature sensor service.
  ******************************************************************************
  */

#ifndef __TEMPERATURE_SERVICE_H
#define __TEMPERATURE_SERVICE_H

#include "main.h"

void TemperatureSensorService_Init(void);
ErrorStatus TemperatureSensorService_GetRecentDs18b20(
  int16_t*,
  uint8_t,
  uint8_t*
);

#endif /* __TEMPERATURE_SERVICE_H */
