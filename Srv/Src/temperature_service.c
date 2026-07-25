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
#define MEMORY_REPORT_CYCLES          10U

static SensorSnapshot_TypeDef sensorSnapshots[SENSOR_SERVICE_MAX_DEVICES];
static uint8_t sensorSnapshotCount;
static BaseType_t bmx280Registered;
static BaseType_t bmx680Registered;

static void temperatureSensorService_Task(void*);
static ErrorStatus temperatureSensorService_MeasureDs18b20(int16_t*, uint8_t*);
static ErrorStatus temperatureSensorService_MeasureBmx280(void);
static ErrorStatus temperatureSensorService_MeasureBmx680(void);
static void temperatureSensorService_UpdateSnapshots(
  const int16_t*,
  uint8_t,
  ErrorStatus,
  ErrorStatus,
  ErrorStatus
);
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
    } else {
      bmx280Registered = pdTRUE;
    }
    if (BMx680_Init(Get_BoschDevice(BMX680_MODEL)) != SUCCESS) {
      FLAG_SET(_PREG_, _PR_BMX680);
    } else {
      bmx680Registered = pdTRUE;
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
void TemperatureSensorService_GetCounts(SensorCounts_TypeDef* counts) {
  if (counts == NULL) return;
  *counts = (SensorCounts_TypeDef){0};
  taskENTER_CRITICAL();
  counts->all = sensorSnapshotCount;
  for (uint8_t i = 0U; i < sensorSnapshotCount; i++) {
    uint8_t capabilities = sensorSnapshots[i].capabilities;
    if ((capabilities & SENSOR_CAPABILITY_TEMPERATURE) != 0U) counts->temperature++;
    if ((capabilities & SENSOR_CAPABILITY_PRESSURE) != 0U) counts->pressure++;
    if ((capabilities & SENSOR_CAPABILITY_HUMIDITY) != 0U) counts->humidity++;
  }
  taskEXIT_CRITICAL();
}





// -------------------------------------------------------------
ErrorStatus TemperatureSensorService_GetSnapshot(
  uint8_t sensorNumber,
  SensorSnapshot_TypeDef* snapshot
) {
  if ((sensorNumber == 0U) || (snapshot == NULL)) return (ERROR);
  ErrorStatus status = ERROR;
  taskENTER_CRITICAL();
  if (sensorNumber <= sensorSnapshotCount) {
    *snapshot = sensorSnapshots[sensorNumber - 1U];
    status = SUCCESS;
  }
  taskEXIT_CRITICAL();
  return (status);
}





// -------------------------------------------------------------
ErrorStatus TemperatureSensorService_GetByCapability(
  SensorCapability_TypeDef capability,
  uint8_t sensorNumber,
  SensorSnapshot_TypeDef* snapshot
) {
  if ((sensorNumber == 0U) || (snapshot == NULL)) return (ERROR);
  uint8_t match = 0U;
  ErrorStatus status = ERROR;
  taskENTER_CRITICAL();
  for (uint8_t i = 0U; i < sensorSnapshotCount; i++) {
    if ((sensorSnapshots[i].capabilities & capability) != 0U) {
      match++;
      if (match == sensorNumber) {
        *snapshot = sensorSnapshots[i];
        status = SUCCESS;
        break;
      }
    }
  }
  taskEXIT_CRITICAL();
  return (status);
}




// -------------------------------------------------------------
static void temperatureSensorService_Task(void* parameters) {
  (void)parameters;
  TickType_t lastWakeTime = xTaskGetTickCount();
  uint8_t memoryReportCounter = 0U;

  while (1) {
    int16_t ds18b20Temperatures[SENSOR_SERVICE_MAX_DS18B20];
    uint8_t ds18b20Count = 0U;
    ErrorStatus ds18b20Status = temperatureSensorService_MeasureDs18b20(
      ds18b20Temperatures,
      &ds18b20Count
    );
    ErrorStatus bmx280Status = temperatureSensorService_MeasureBmx280();
    ErrorStatus bmx680Status = temperatureSensorService_MeasureBmx680();
    temperatureSensorService_UpdateSnapshots(
      ds18b20Temperatures,
      ds18b20Count,
      ds18b20Status,
      bmx280Status,
      bmx680Status
    );
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
        SENSOR_SERVICE_MAX_DS18B20,
        count
      ) != SUCCESS) {
    return (ERROR);
  }

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
static void temperatureSensorService_UpdateSnapshots(
  const int16_t* ds18b20Temperatures,
  uint8_t ds18b20Count,
  ErrorStatus ds18b20Status,
  ErrorStatus bmx280Status,
  ErrorStatus bmx680Status
) {
  SensorSnapshot_TypeDef updated[SENSOR_SERVICE_MAX_DEVICES];
  uint8_t count = 0U;

  if (ds18b20Status == SUCCESS) {
    for (uint8_t i = 0U;
         (i < ds18b20Count) && (count < SENSOR_SERVICE_MAX_DEVICES);
         i++) {
      updated[count++] = (SensorSnapshot_TypeDef){
        .model = SENSOR_MODEL_DS18B20,
        .capabilities = SENSOR_CAPABILITY_TEMPERATURE,
        .dataValid = pdTRUE,
        .temperature = ds18b20Temperatures[i]
      };
    }
  } else {
    taskENTER_CRITICAL();
    while ((count < sensorSnapshotCount)
        && (sensorSnapshots[count].model == SENSOR_MODEL_DS18B20)) {
      updated[count] = sensorSnapshots[count];
      updated[count].dataValid = pdFALSE;
      count++;
    }
    taskEXIT_CRITICAL();
  }

  if ((bmx280Registered == pdTRUE) && (count < SENSOR_SERVICE_MAX_DEVICES)) {
    BMxX80_TypeDef* device = Get_BoschDevice(BMX280_MODEL);
    uint8_t capabilities = SENSOR_CAPABILITY_TEMPERATURE
      | SENSOR_CAPABILITY_PRESSURE;
    if (device->DevID == BME280_ID) capabilities |= SENSOR_CAPABILITY_HUMIDITY;
    updated[count++] = (SensorSnapshot_TypeDef){
      .model = (device->DevID == BME280_ID)
        ? SENSOR_MODEL_BME280
        : SENSOR_MODEL_BMP280,
      .capabilities = capabilities,
      .dataValid = (bmx280Status == SUCCESS) ? pdTRUE : pdFALSE,
      .temperature = device->Results.temperature,
      .pressure = device->Results.pressure,
      .humidity = device->Results.humidity
    };
  }

  if ((bmx680Registered == pdTRUE) && (count < SENSOR_SERVICE_MAX_DEVICES)) {
    BMxX80_TypeDef* device = Get_BoschDevice(BMX680_MODEL);
    updated[count++] = (SensorSnapshot_TypeDef){
      .model = SENSOR_MODEL_BME680,
      .capabilities = SENSOR_CAPABILITY_TEMPERATURE
        | SENSOR_CAPABILITY_PRESSURE
        | SENSOR_CAPABILITY_HUMIDITY,
      .dataValid = (bmx680Status == SUCCESS) ? pdTRUE : pdFALSE,
      .temperature = device->Results.temperature,
      .pressure = device->Results.pressure,
      .humidity = device->Results.humidity
    };
  }

  taskENTER_CRITICAL();
  for (uint8_t i = 0U; i < count; i++) sensorSnapshots[i] = updated[i];
  sensorSnapshotCount = count;
  taskEXIT_CRITICAL();
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
