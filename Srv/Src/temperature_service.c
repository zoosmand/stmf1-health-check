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
#define MEMORY_REPORT_CYCLES          10U

static int16_t recentDs18b20Temperatures[MAX_REPORTED_DS18B20_DEVICES];
static uint8_t recentDs18b20Count;
static BaseType_t recentDs18b20Valid;

static void temperatureSensorService_Task(void*);
static ErrorStatus temperatureSensorService_MeasureDs18b20(int16_t*, uint8_t*);
static ErrorStatus temperatureSensorService_MeasureBmx280(void);
static ErrorStatus temperatureSensorService_MeasureBmx680(void);
static void temperatureSensorService_PrintMeasurements(
  const int16_t*,
  uint8_t,
  ErrorStatus,
  ErrorStatus,
  ErrorStatus
);
static void temperatureSensorService_ReportMemory(void);
static UBaseType_t temperatureSensorService_GetStackMargin(const char*);
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
  } else {
    FLAG_SET(_PREG_, _PR_BMX280);
    FLAG_SET(_PREG_, _PR_BMX680);
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
  uint8_t memoryReportCounter = 0U;

  while (1) {
    int16_t ds18b20Temperatures[MAX_REPORTED_DS18B20_DEVICES];
    uint8_t ds18b20Count = 0U;
    ErrorStatus ds18b20Status = temperatureSensorService_MeasureDs18b20(
      ds18b20Temperatures,
      &ds18b20Count
    );
    ErrorStatus bmx280Status = temperatureSensorService_MeasureBmx280();
    ErrorStatus bmx680Status = temperatureSensorService_MeasureBmx680();
    temperatureSensorService_PrintMeasurements(
      ds18b20Temperatures,
      ds18b20Count,
      ds18b20Status,
      bmx280Status,
      bmx680Status
    );

    memoryReportCounter++;
    if (memoryReportCounter >= MEMORY_REPORT_CYCLES) {
      memoryReportCounter = 0U;
      temperatureSensorService_ReportMemory();
    }
    vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(TEMPERATURE_SERVICE_PERIOD_MS));
  }
}




// -------------------------------------------------------------
static ErrorStatus temperatureSensorService_MeasureDs18b20(
  int16_t* temperatures,
  uint8_t* count
) {
  if (DS18B20_MeasureTemperatures(
        temperatures,
        MAX_REPORTED_DS18B20_DEVICES,
        count
      ) != SUCCESS) {
    return (ERROR);
  }

  taskENTER_CRITICAL();
  for (uint8_t i = 0U; i < *count; i++) {
    recentDs18b20Temperatures[i] = temperatures[i];
  }
  recentDs18b20Count = *count;
  recentDs18b20Valid = pdTRUE;
  taskEXIT_CRITICAL();

  return (SUCCESS);
}




// -------------------------------------------------------------
static ErrorStatus temperatureSensorService_MeasureBmx280(void) {
  if (FLAG_CHECK(_PREG_, _PR_BMX280)) return (ERROR);

  BMxX80_TypeDef* device = Get_BoschDevice(BMX280_MODEL);
  return BMx280_Measurement(device);
}




// -------------------------------------------------------------
static ErrorStatus temperatureSensorService_MeasureBmx680(void) {
  if (FLAG_CHECK(_PREG_, _PR_BMX680)) return (ERROR);

  BMxX80_TypeDef* device = Get_BoschDevice(BMX680_MODEL);
  return BMx680_Measurement(device);
}




// -------------------------------------------------------------
static void temperatureSensorService_PrintMeasurements(
  const int16_t* ds18b20Temperatures,
  uint8_t ds18b20Count,
  ErrorStatus ds18b20Status,
  ErrorStatus bmx280Status,
  ErrorStatus bmx680Status
) {
  printf("DS18B20: ");
  if (ds18b20Status == SUCCESS) {
    for (uint8_t i = 0U; i < ds18b20Count; i++) {
      if (i > 0U) printf(", ");
      temperatureSensorService_PrintCentiDegrees(ds18b20Temperatures[i]);
    }
    printf(" C");
  } else {
    printf("conversion error");
  }
  printf("\n");

  if (bmx280Status == SUCCESS) {
    BMxX80_TypeDef* device = Get_BoschDevice(BMX280_MODEL);
    printf((device->DevID == BME280_ID) ? "BME280: " : "BMP280: ");
    temperatureSensorService_PrintCentiDegrees(device->Results.temperature);
    printf(" C, %lu Pa", (unsigned long)device->Results.pressure);
    if (device->DevID == BME280_ID) {
      printf(
        ", %lu.%03lu %%RH",
        (unsigned long)(device->Results.humidity / 1000U),
        (unsigned long)(device->Results.humidity % 1000U)
      );
    }
  } else {
    printf("BMx280: conversion error");
  }
  printf("\n");

  if (bmx680Status == SUCCESS) {
    BMxX80_TypeDef* device = Get_BoschDevice(BMX680_MODEL);
    printf("BME680: ");
    temperatureSensorService_PrintCentiDegrees(device->Results.temperature);
    printf(
      " C, %lu Pa, %lu.%03lu %%RH",
      (unsigned long)device->Results.pressure,
      (unsigned long)(device->Results.humidity / 1000U),
      (unsigned long)(device->Results.humidity % 1000U)
    );
  } else {
    printf("BME680: conversion error");
  }
  printf("\n");
}




// -------------------------------------------------------------
static void temperatureSensorService_ReportMemory(void) {
  printf(
    "Memory heap=%u min=%u stack=%u/%u/%u/%u\n",
    (unsigned int)xPortGetFreeHeapSize(),
    (unsigned int)xPortGetMinimumEverFreeHeapSize(),
    (unsigned int)temperatureSensorService_GetStackMargin("Heart Beat"),
    (unsigned int)temperatureSensorService_GetStackMargin("OW Bus Init"),
    (unsigned int)uxTaskGetStackHighWaterMark(NULL),
    (unsigned int)temperatureSensorService_GetStackMargin("TCP Commands")
  );
}




// -------------------------------------------------------------
static UBaseType_t temperatureSensorService_GetStackMargin(const char* taskName) {
  TaskHandle_t task = xTaskGetHandle(taskName);
  return (task != NULL) ? uxTaskGetStackHighWaterMark(task) : 0U;
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
