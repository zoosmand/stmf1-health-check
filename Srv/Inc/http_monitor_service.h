/**
  ******************************************************************************
  * @file           : http_monitor_service.h
  * @brief          : Periodic HTTP resource health-monitor interface.
  * @project        : STM32F1 Health Check Device
  * @platform       : STMicroelectronics STM32F103C8
  * @created        : 27.07.2026
  ******************************************************************************
  * @attention
  * @copyright  : 2017-2026, Dmitry Slobodchikov
  ******************************************************************************
  */

#ifndef __HTTP_MONITOR_SERVICE_H
#define __HTTP_MONITOR_SERVICE_H

#include "main.h"
#include "device_health.h"

#define HTTP_MONITOR_HOST "hvm-a.ic.local"
#define HTTP_MONITOR_PORT_NUMBER 3000
#define HTTP_MONITOR_PORT ((uint16_t)HTTP_MONITOR_PORT_NUMBER)
#define HTTP_MONITOR_PATH "/"

typedef enum {
  HTTP_MONITOR_ERROR_NONE = 0U,
  HTTP_MONITOR_ERROR_DNS,
  HTTP_MONITOR_ERROR_SOCKET,
  HTTP_MONITOR_ERROR_CONNECT,
  HTTP_MONITOR_ERROR_SEND,
  HTTP_MONITOR_ERROR_TIMEOUT,
  HTTP_MONITOR_ERROR_RESPONSE,
  HTTP_MONITOR_ERROR_STATUS
} HttpMonitorError_TypeDef;

/**
  * @brief Latest result of the configured HTTP resource check.
  */
typedef struct {
  DeviceHealth_TypeDef health;
  uint16_t statusCode;
  uint8_t resolvedAddress[4];
} HttpMonitorSnapshot_TypeDef;

/**
  * @brief Create the periodic HTTP monitor task and DNS timeout timer.
  */
void HttpMonitorService_Init(void);

/**
  * @brief Copy the latest HTTP monitor result.
  * @param snapshot (HttpMonitorSnapshot_TypeDef*) Destination snapshot.
  */
void HttpMonitorService_GetSnapshot(HttpMonitorSnapshot_TypeDef* snapshot);

#endif /* __HTTP_MONITOR_SERVICE_H */
