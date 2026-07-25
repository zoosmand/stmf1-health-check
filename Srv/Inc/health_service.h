/**
  ******************************************************************************
  * @file           : health_service.h
  * @brief          : Internal health supervision and watchdog service.
  ******************************************************************************
  */

#ifndef __HEALTH_SERVICE_H
#define __HEALTH_SERVICE_H

#include "main.h"

typedef enum {
  HEALTH_COMPONENT_HEART_BEAT  = (1UL << 0U),
  HEALTH_COMPONENT_TEMPERATURE = (1UL << 1U),
  HEALTH_COMPONENT_TCP         = (1UL << 2U)
} HealthComponent_TypeDef;

void HealthService_Init(void);
void HealthService_Register(HealthComponent_TypeDef);
void HealthService_Report(HealthComponent_TypeDef);

/**
 * @brief Latch an emergency condition and stop reloading the watchdog.
 *
 * The IWDG resets the controller within four seconds after this call.
 */
void system_error(void);

#endif /* __HEALTH_SERVICE_H */
