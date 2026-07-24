/**
  ******************************************************************************
  * @file           : temperature_service.c
  * @brief          : Sequential periodic temperature sensor service.
  ******************************************************************************
  */

#include "temperature_service.h"
#include "ds18b20.h"
#include "bmx280.h"
#include "bmx680.h"

#define TEMPERATURE_SERVICE_PERIOD_MS 7000U
#define MAX_REPORTED_DS18B20_DEVICES  2U

static int16_t recentDs18b20Temperatures[MAX_REPORTED_DS18B20_DEVICES];
static uint8_t recentDs18b20Count;
static BaseType_t recentDs18b20Valid;

static void temperatureSensorService_Task(void*);
static void temperatureSensorService_MeasureDs18b20(void);
static void temperatureSensorService_MeasureBmx280(void);
static void temperatureSensorService_MeasureBmx680(void);
static void temperatureSensorService_PrintCentiDegrees(int32_t);




// -------------------------------------------------------------
void TemperatureSensorService_Init(void) {
  if (!FLAG_CHECK(_PREG_, _PR_I2C1_BUS)) {
    if (BMx280_Init(Get_BoschDevice(BMX280_MODEL)) != SUCCESS) {
      FLAG_SET(_PREG_, _PR_BMX280);
    }
    if (BMx680_Init(Get_BoschDevice(BMX680_MODEL)) != SUCCESS) {
      FLAG_SET(_PREG_, _PR_BMX680);
    }
  }

  static StaticTask_t taskControlBlock;
  static StackType_t taskStack[configMINIMAL_STACK_SIZE * 4U];
  (void)xTaskCreateStatic(
    temperatureSensorService_Task,
    "Temperature",
    configMINIMAL_STACK_SIZE * 4U,
    NULL,
    configMAX_PRIORITIES - 2U,
    taskStack,
    &taskControlBlock
  );
}




// -------------------------------------------------------------
ErrorStatus TemperatureSensorService_GetRecentDs18b20(
  int16_t* temperatures,
  uint8_t capacity,
  uint8_t* count
) {
  if ((temperatures == NULL) || (count == NULL) || (capacity == 0U)) return (ERROR);
  *count = 0U;

  taskENTER_CRITICAL();
  if (recentDs18b20Valid == pdFALSE) {
    taskEXIT_CRITICAL();
    return (ERROR);
  }

  uint8_t copyCount = (recentDs18b20Count < capacity)
    ? recentDs18b20Count
    : capacity;
  for (uint8_t i = 0U; i < copyCount; i++) {
    temperatures[i] = recentDs18b20Temperatures[i];
  }
  *count = copyCount;
  taskEXIT_CRITICAL();

  return (copyCount > 0U) ? SUCCESS : ERROR;
}




// -------------------------------------------------------------
static void temperatureSensorService_Task(void* parameters) {
  (void)parameters;
  TickType_t lastWakeTime = xTaskGetTickCount();

  while (1) {
    temperatureSensorService_MeasureDs18b20();
    temperatureSensorService_MeasureBmx280();
    temperatureSensorService_MeasureBmx680();
    vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(TEMPERATURE_SERVICE_PERIOD_MS));
  }
}




// -------------------------------------------------------------
static void temperatureSensorService_MeasureDs18b20(void) {
  int16_t temperatures[MAX_REPORTED_DS18B20_DEVICES];
  uint8_t count = 0U;

  if (DS18B20_MeasureTemperatures(
        temperatures,
        MAX_REPORTED_DS18B20_DEVICES,
        &count
      ) != SUCCESS) {
    printf("DS18B20: conversion error\n");
    return;
  }

  taskENTER_CRITICAL();
  for (uint8_t i = 0U; i < count; i++) {
    recentDs18b20Temperatures[i] = temperatures[i];
  }
  recentDs18b20Count = count;
  recentDs18b20Valid = pdTRUE;
  taskEXIT_CRITICAL();

  printf("DS18B20:");
  for (uint8_t i = 0U; i < count; i++) {
    printf(" ");
    temperatureSensorService_PrintCentiDegrees(temperatures[i]);
  }
  printf(" C\n");
}




// -------------------------------------------------------------
static void temperatureSensorService_MeasureBmx280(void) {
  if (FLAG_CHECK(_PREG_, _PR_BMX280)) {
    printf("BMx280: unavailable\n");
    return;
  }

  BMxX80_TypeDef* device = Get_BoschDevice(BMX280_MODEL);
  if (BMx280_Measurement(device) != SUCCESS) {
    printf("BMx280: conversion error\n");
    return;
  }

  printf("BMx280: ");
  temperatureSensorService_PrintCentiDegrees(device->Results.temperature);
  printf(
    " C, %lu Pa, %u.%03u %%RH\n",
    (unsigned long)device->Results.pressure,
    (unsigned int)(device->Results.humidity / 1024U),
    (unsigned int)(((uint32_t)(device->Results.humidity % 1024U) * 1000U) / 1024U)
  );
}




// -------------------------------------------------------------
static void temperatureSensorService_MeasureBmx680(void) {
  if (FLAG_CHECK(_PREG_, _PR_BMX680)) {
    printf("BMx680: unavailable\n");
    return;
  }

  BMxX80_TypeDef* device = Get_BoschDevice(BMX680_MODEL);
  if (BMx680_Measurement(device) != SUCCESS) {
    printf("BMx680: conversion error\n");
    return;
  }

  printf("BMx680: ");
  temperatureSensorService_PrintCentiDegrees(device->Results.temperature);
  printf(" C\n");
}




// -------------------------------------------------------------
static void temperatureSensorService_PrintCentiDegrees(int32_t centiDegrees) {
  uint32_t magnitude = (centiDegrees < 0)
    ? (uint32_t)(-centiDegrees)
    : (uint32_t)centiDegrees;
  printf(
    "%s%lu.%02lu",
    (centiDegrees < 0) ? "-" : "",
    (unsigned long)(magnitude / 100U),
    (unsigned long)(magnitude % 100U)
  );
}
