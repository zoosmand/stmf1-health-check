/**
  ******************************************************************************
  * @file           : http_monitor_service.c
  * @brief          : Periodic HTTP resource health-monitor implementation.
  * @project        : STM32F1 Health Check Device
  * @platform       : STMicroelectronics STM32F103C8
  * @created        : 27.07.2026
  ******************************************************************************
  * @attention
  * @copyright  : 2017-2026, Dmitry Slobodchikov
  ******************************************************************************
  */

#include "http_monitor_service.h"
#include "DNS/dns.h"
#include "socket.h"
#include <string.h>

#define HTTP_MONITOR_SOCKET              4U
#define HTTP_MONITOR_PERIOD_MS       60000U
#define HTTP_MONITOR_RESPONSE_MS      5000U
#define HTTP_MONITOR_LINE_SIZE          64U
#define HTTP_MONITOR_FAILURE_THRESHOLD   3U

static HttpMonitorSnapshot_TypeDef httpMonitorSnapshot = {
  .health = {
    .state = DEVICE_HEALTH_INITIALIZING
  }
};

static void httpMonitorService_Task(void* parameters);
static void httpMonitorService_DnsTimer(TimerHandle_t timer);
static ErrorStatus httpMonitorService_Check(
  uint8_t* address,
  uint16_t* statusCode,
  HttpMonitorError_TypeDef* error
);
static ErrorStatus httpMonitorService_ReadStatus(
  uint16_t* statusCode,
  HttpMonitorError_TypeDef* error
);
static ErrorStatus httpMonitorService_ParseStatus(
  const char* line,
  uint16_t length,
  uint16_t* statusCode
);
static void httpMonitorService_Record(
  ErrorStatus result,
  HttpMonitorError_TypeDef error,
  const uint8_t* address,
  uint16_t statusCode
);


// -------------------------------------------------------------
void HttpMonitorService_Init(void) {
  static StaticTask_t taskControlBlock;
  static StackType_t taskStack[configMINIMAL_STACK_SIZE * 2U];
  static StaticTimer_t dnsTimerStorage;
  TimerHandle_t dnsTimer;

  dnsTimer = xTimerCreateStatic(
    "DNS tick",
    pdMS_TO_TICKS(1000U),
    pdTRUE,
    NULL,
    httpMonitorService_DnsTimer,
    &dnsTimerStorage
  );
  if (dnsTimer != NULL) (void)xTimerStart(dnsTimer, 0U);

  (void)xTaskCreateStatic(
    httpMonitorService_Task,
    "HTTP Monitor",
    configMINIMAL_STACK_SIZE * 2U,
    NULL,
    configMAX_PRIORITIES - 3U,
    taskStack,
    &taskControlBlock
  );
}




// -------------------------------------------------------------
void HttpMonitorService_GetSnapshot(HttpMonitorSnapshot_TypeDef* snapshot) {
  if (snapshot == NULL) return;
  taskENTER_CRITICAL();
  *snapshot = httpMonitorSnapshot;
  taskEXIT_CRITICAL();
}




// -------------------------------------------------------------
static void httpMonitorService_Task(void* parameters) {
  TickType_t lastWakeTime = xTaskGetTickCount();
  (void)parameters;

  while (1) {
    uint8_t address[4] = {0U};
    uint16_t statusCode = 0U;
    HttpMonitorError_TypeDef error = HTTP_MONITOR_ERROR_NONE;
    ErrorStatus result = httpMonitorService_Check(
      address,
      &statusCode,
      &error
    );

    httpMonitorService_Record(result, error, address, statusCode);
    printf(
      "HTTP check: %s, status:%u, error:%u\n",
      (result == SUCCESS) ? "OK" : "FAILED",
      statusCode,
      (unsigned int)error
    );
    vTaskDelayUntil(
      &lastWakeTime,
      pdMS_TO_TICKS(HTTP_MONITOR_PERIOD_MS)
    );
  }
}




// -------------------------------------------------------------
static void httpMonitorService_DnsTimer(TimerHandle_t timer) {
  (void)timer;
  DNS_time_handler();
}




// -------------------------------------------------------------
static ErrorStatus httpMonitorService_Check(
  uint8_t* address,
  uint16_t* statusCode,
  HttpMonitorError_TypeDef* error
) {
  static const uint8_t host[] = HTTP_MONITOR_HOST;
  static uint8_t request[] =
    "OPTIONS " HTTP_MONITOR_PATH " HTTP/1.1\r\n"
    "Host: " HTTP_MONITOR_HOST ":3000\r\n"
    "Connection: close\r\n"
    "\r\n";
  int32_t result;
  uint8_t dnsServer[4];

  W5500_GetDnsServer(dnsServer);
  if (DNS_run(dnsServer, (uint8_t*)host, address) != 1) {
    *error = HTTP_MONITOR_ERROR_DNS;
    return (ERROR);
  }

  (void)close(HTTP_MONITOR_SOCKET);
  if (socket(HTTP_MONITOR_SOCKET, Sn_MR_TCP, 0U, 0U)
      != HTTP_MONITOR_SOCKET) {
    *error = HTTP_MONITOR_ERROR_SOCKET;
    return (ERROR);
  }

  if (connect(HTTP_MONITOR_SOCKET, address, HTTP_MONITOR_PORT) != SOCK_OK) {
    *error = HTTP_MONITOR_ERROR_CONNECT;
    (void)close(HTTP_MONITOR_SOCKET);
    return (ERROR);
  }

  result = send(
    HTTP_MONITOR_SOCKET,
    request,
    (uint16_t)(sizeof(request) - 1U)
  );
  if (result != (int32_t)(sizeof(request) - 1U)) {
    *error = HTTP_MONITOR_ERROR_SEND;
    (void)close(HTTP_MONITOR_SOCKET);
    return (ERROR);
  }

  result = httpMonitorService_ReadStatus(statusCode, error);
  (void)disconnect(HTTP_MONITOR_SOCKET);
  (void)close(HTTP_MONITOR_SOCKET);
  if (result != SUCCESS) {
    return (ERROR);
  }
  if (*statusCode != 200U) {
    *error = HTTP_MONITOR_ERROR_STATUS;
    return (ERROR);
  }

  return (SUCCESS);
}




// -------------------------------------------------------------
static ErrorStatus httpMonitorService_ReadStatus(
  uint16_t* statusCode,
  HttpMonitorError_TypeDef* error
) {
  char line[HTTP_MONITOR_LINE_SIZE];
  uint16_t length = 0U;
  TickType_t started = xTaskGetTickCount();

  while ((xTaskGetTickCount() - started)
         < pdMS_TO_TICKS(HTTP_MONITOR_RESPONSE_MS)) {
    uint16_t available = getSn_RX_RSR(HTTP_MONITOR_SOCKET);

    if (available > 0U) {
      uint8_t byte;
      if (recv(HTTP_MONITOR_SOCKET, &byte, 1U) != 1) {
        *error = HTTP_MONITOR_ERROR_RESPONSE;
        return (ERROR);
      }
      if (length >= (sizeof(line) - 1U)) {
        *error = HTTP_MONITOR_ERROR_RESPONSE;
        return (ERROR);
      }
      line[length++] = (char)byte;
      if ((length >= 2U)
          && (line[length - 2U] == '\r')
          && (line[length - 1U] == '\n')) {
        line[length] = '\0';
        if (httpMonitorService_ParseStatus(line, length, statusCode)
            != SUCCESS) {
          *error = HTTP_MONITOR_ERROR_RESPONSE;
          return (ERROR);
        }
        return (SUCCESS);
      }
    } else {
      uint8_t state = getSn_SR(HTTP_MONITOR_SOCKET);
      if ((state == SOCK_CLOSED) || (state == SOCK_CLOSE_WAIT)) {
        *error = HTTP_MONITOR_ERROR_RESPONSE;
        return (ERROR);
      }
      vTaskDelay(pdMS_TO_TICKS(10U));
    }
  }

  *error = HTTP_MONITOR_ERROR_TIMEOUT;
  return (ERROR);
}




// -------------------------------------------------------------
static ErrorStatus httpMonitorService_ParseStatus(
  const char* line,
  uint16_t length,
  uint16_t* statusCode
) {
  if ((line == NULL) || (statusCode == NULL) || (length < 14U)) {
    return (ERROR);
  }
  if ((memcmp(line, "HTTP/1.", 7U) != 0)
      || ((line[7] != '0') && (line[7] != '1'))
      || (line[8] != ' ')
      || (line[9] < '0') || (line[9] > '9')
      || (line[10] < '0') || (line[10] > '9')
      || (line[11] < '0') || (line[11] > '9')) {
    return (ERROR);
  }

  *statusCode = (uint16_t)(
    ((uint16_t)(line[9] - '0') * 100U)
    + ((uint16_t)(line[10] - '0') * 10U)
    + (uint16_t)(line[11] - '0')
  );
  return (SUCCESS);
}




// -------------------------------------------------------------
static void httpMonitorService_Record(
  ErrorStatus result,
  HttpMonitorError_TypeDef error,
  const uint8_t* address,
  uint16_t statusCode
) {
  TickType_t now = xTaskGetTickCount();

  taskENTER_CRITICAL();
  httpMonitorSnapshot.health.lastAttempt = now;
  httpMonitorSnapshot.statusCode = statusCode;
  memcpy(httpMonitorSnapshot.resolvedAddress, address, 4U);

  if (result == SUCCESS) {
    httpMonitorSnapshot.health.lastSuccess = now;
    httpMonitorSnapshot.health.consecutiveFailures = 0U;
    httpMonitorSnapshot.health.lastError = HTTP_MONITOR_ERROR_NONE;
    httpMonitorSnapshot.health.state = DEVICE_HEALTH_AVAILABLE;
  } else {
    if (httpMonitorSnapshot.health.consecutiveFailures < UINT16_MAX) {
      httpMonitorSnapshot.health.consecutiveFailures++;
    }
    httpMonitorSnapshot.health.lastError = (uint8_t)error;
    httpMonitorSnapshot.health.state =
      (httpMonitorSnapshot.health.consecutiveFailures
       >= HTTP_MONITOR_FAILURE_THRESHOLD)
      ? DEVICE_HEALTH_UNAVAILABLE
      : DEVICE_HEALTH_DEGRADED;
  }
  taskEXIT_CRITICAL();
}
