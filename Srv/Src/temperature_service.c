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
#include <string.h>

#define TEMPERATURE_SERVICE_PERIOD_MS 7000U
#define SENSOR_STALE_PERIOD_MS        (TEMPERATURE_SERVICE_PERIOD_MS * 3U)
#define SENSOR_FAILURE_THRESHOLD      3U
#define MEMORY_REPORT_CYCLES          10U

static SensorSnapshot_TypeDef sensorSnapshots[SENSOR_SERVICE_MAX_DEVICES];
static uint8_t sensorSnapshotCount;
static BaseType_t bmx280Registered;
static BaseType_t bmx680Registered;

static void temperatureSensorService_Task(void*);
static ErrorStatus temperatureSensorService_MeasureDs18b20(
  DS18B20_Measurement_TypeDef*,
  uint8_t*
);
static ErrorStatus temperatureSensorService_MeasureBmx280(void);
static ErrorStatus temperatureSensorService_MeasureBmx680(void);
static void temperatureSensorService_UpdateSnapshots(
  const DS18B20_Measurement_TypeDef*,
  uint8_t,
  ErrorStatus,
  ErrorStatus
);
static void temperatureSensorService_PrintMeasurements(
  const DS18B20_Measurement_TypeDef*,
  uint8_t,
  ErrorStatus,
  ErrorStatus
);
static int8_t temperatureSensorService_FindDs18b20(const uint8_t*);
static int8_t temperatureSensorService_FindModel(SensorModel_TypeDef);
static void temperatureSensorService_RecordResult(
  SensorSnapshot_TypeDef*,
  ErrorStatus,
  SensorError_TypeDef,
  TickType_t
);
static SensorError_TypeDef temperatureSensorService_MapDs18b20Error(
  DS18B20_Status_TypeDef
);
static void temperatureSensorService_ApplyAge(SensorSnapshot_TypeDef*);
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
  HealthService_Register(HEALTH_COMPONENT_TEMPERATURE);
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
    temperatureSensorService_ApplyAge(snapshot);
    status = SUCCESS;
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
    DS18B20_Measurement_TypeDef ds18b20Measurements[
      SENSOR_SERVICE_MAX_DS18B20
    ];
    uint8_t ds18b20Count = 0U;
    (void)temperatureSensorService_MeasureDs18b20(
      ds18b20Measurements,
      &ds18b20Count
    );
    ErrorStatus bmx280Status = temperatureSensorService_MeasureBmx280();
    ErrorStatus bmx680Status = temperatureSensorService_MeasureBmx680();
    temperatureSensorService_UpdateSnapshots(
      ds18b20Measurements,
      ds18b20Count,
      bmx280Status,
      bmx680Status
    );
    temperatureSensorService_PrintMeasurements(
      ds18b20Measurements,
      ds18b20Count,
      bmx280Status,
      bmx680Status
    );
    HealthService_Report(HEALTH_COMPONENT_TEMPERATURE);

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
  DS18B20_Measurement_TypeDef* measurements,
  uint8_t* count
) {
  return DS18B20_Measure(
    measurements,
    SENSOR_SERVICE_MAX_DS18B20,
    count
  );
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
  const DS18B20_Measurement_TypeDef* ds18b20Measurements,
  uint8_t ds18b20Count,
  ErrorStatus bmx280Status,
  ErrorStatus bmx680Status
) {
  TickType_t now = xTaskGetTickCount();
  BaseType_t ds18b20Seen[SENSOR_SERVICE_MAX_DEVICES] = {pdFALSE};

  taskENTER_CRITICAL();
  for (uint8_t i = 0U; i < ds18b20Count; i++) {
    int8_t index = temperatureSensorService_FindDs18b20(
      ds18b20Measurements[i].rom
    );
    if ((index < 0) && (sensorSnapshotCount < SENSOR_SERVICE_MAX_DEVICES)) {
      index = (int8_t)sensorSnapshotCount++;
      sensorSnapshots[index] = (SensorSnapshot_TypeDef){
        .model = SENSOR_MODEL_DS18B20,
        .capabilities = SENSOR_CAPABILITY_TEMPERATURE,
        .health = SENSOR_HEALTH_INITIALIZING,
        .lastError = SENSOR_ERROR_NOT_READY
      };
      memcpy(
        sensorSnapshots[index].identity,
        ds18b20Measurements[i].rom,
        sizeof(sensorSnapshots[index].identity)
      );
    }
    if (index >= 0) {
      ds18b20Seen[(uint8_t)index] = pdTRUE;
      ErrorStatus status = (ds18b20Measurements[i].status == DS18B20_STATUS_OK)
        ? SUCCESS
        : ERROR;
      if (status == SUCCESS) {
        sensorSnapshots[index].temperature = ds18b20Measurements[i].temperature;
      }
      temperatureSensorService_RecordResult(
        &sensorSnapshots[index],
        status,
        temperatureSensorService_MapDs18b20Error(ds18b20Measurements[i].status),
        now
      );
    }
  }
  for (uint8_t i = 0U; i < sensorSnapshotCount; i++) {
    if ((sensorSnapshots[i].model == SENSOR_MODEL_DS18B20)
        && (ds18b20Seen[i] == pdFALSE)) {
      temperatureSensorService_RecordResult(
        &sensorSnapshots[i],
        ERROR,
        SENSOR_ERROR_MISSING,
        now
      );
      sensorSnapshots[i].health = SENSOR_HEALTH_MISSING;
      sensorSnapshots[i].dataValid = pdFALSE;
    }
  }

  if (bmx280Registered == pdTRUE) {
    BMxX80_TypeDef* device = Get_BoschDevice(BMX280_MODEL);
    int8_t index = temperatureSensorService_FindModel(
      (device->DevID == BME280_ID) ? SENSOR_MODEL_BME280 : SENSOR_MODEL_BMP280
    );
    if ((index < 0) && (sensorSnapshotCount < SENSOR_SERVICE_MAX_DEVICES)) {
      index = (int8_t)sensorSnapshotCount++;
      sensorSnapshots[index] = (SensorSnapshot_TypeDef){
        .model = (device->DevID == BME280_ID)
          ? SENSOR_MODEL_BME280
          : SENSOR_MODEL_BMP280,
        .capabilities = SENSOR_CAPABILITY_TEMPERATURE
          | SENSOR_CAPABILITY_PRESSURE
          | ((device->DevID == BME280_ID) ? SENSOR_CAPABILITY_HUMIDITY : 0U),
        .health = SENSOR_HEALTH_INITIALIZING,
        .lastError = SENSOR_ERROR_NOT_READY
      };
      sensorSnapshots[index].serialNumber = device->UniqueID;
    }
    if (index >= 0) {
      sensorSnapshots[index].temperature = device->Results.temperature;
      sensorSnapshots[index].pressure = device->Results.pressure;
      sensorSnapshots[index].humidity = device->Results.humidity;
      temperatureSensorService_RecordResult(
        &sensorSnapshots[index],
        bmx280Status,
        SENSOR_ERROR_CONVERSION,
        now
      );
    }
  }

  if (bmx680Registered == pdTRUE) {
    BMxX80_TypeDef* device = Get_BoschDevice(BMX680_MODEL);
    int8_t index = temperatureSensorService_FindModel(SENSOR_MODEL_BME680);
    if ((index < 0) && (sensorSnapshotCount < SENSOR_SERVICE_MAX_DEVICES)) {
      index = (int8_t)sensorSnapshotCount++;
      sensorSnapshots[index] = (SensorSnapshot_TypeDef){
        .model = SENSOR_MODEL_BME680,
        .capabilities = SENSOR_CAPABILITY_TEMPERATURE
          | SENSOR_CAPABILITY_PRESSURE
          | SENSOR_CAPABILITY_HUMIDITY,
        .health = SENSOR_HEALTH_INITIALIZING,
        .lastError = SENSOR_ERROR_NOT_READY
      };
      sensorSnapshots[index].serialNumber = device->UniqueID;
    }
    if (index >= 0) {
      sensorSnapshots[index].temperature = device->Results.temperature;
      sensorSnapshots[index].pressure = device->Results.pressure;
      sensorSnapshots[index].humidity = device->Results.humidity;
      temperatureSensorService_RecordResult(
        &sensorSnapshots[index],
        bmx680Status,
        SENSOR_ERROR_CONVERSION,
        now
      );
    }
  }
  taskEXIT_CRITICAL();
}




// -------------------------------------------------------------
static int8_t temperatureSensorService_FindDs18b20(const uint8_t* rom) {
  for (uint8_t i = 0U; i < sensorSnapshotCount; i++) {
    if ((sensorSnapshots[i].model == SENSOR_MODEL_DS18B20)
        && (memcmp(sensorSnapshots[i].identity, rom, 8U) == 0)) {
      return ((int8_t)i);
    }
  }
  return (-1);
}




// -------------------------------------------------------------
static int8_t temperatureSensorService_FindModel(SensorModel_TypeDef model) {
  for (uint8_t i = 0U; i < sensorSnapshotCount; i++) {
    if (sensorSnapshots[i].model == model) return ((int8_t)i);
  }
  return (-1);
}




// -------------------------------------------------------------
static void temperatureSensorService_RecordResult(
  SensorSnapshot_TypeDef* snapshot,
  ErrorStatus status,
  SensorError_TypeDef error,
  TickType_t now
) {
  snapshot->lastAttempt = now;
  if (status == SUCCESS) {
    snapshot->dataValid = pdTRUE;
    snapshot->lastSuccess = now;
    snapshot->consecutiveFailures = 0U;
    snapshot->health = SENSOR_HEALTH_HEALTHY;
    snapshot->lastError = SENSOR_ERROR_NONE;
    return;
  }

  if (snapshot->consecutiveFailures < UINT16_MAX) {
    snapshot->consecutiveFailures++;
  }
  snapshot->lastError = error;
  snapshot->health = (snapshot->consecutiveFailures >= SENSOR_FAILURE_THRESHOLD)
    ? SENSOR_HEALTH_FAILED
    : SENSOR_HEALTH_DEGRADED;
  if (snapshot->health == SENSOR_HEALTH_FAILED) snapshot->dataValid = pdFALSE;
}




// -------------------------------------------------------------
static SensorError_TypeDef temperatureSensorService_MapDs18b20Error(
  DS18B20_Status_TypeDef status
) {
  switch (status) {
    case DS18B20_STATUS_OK:      return (SENSOR_ERROR_NONE);
    case DS18B20_STATUS_TIMEOUT: return (SENSOR_ERROR_TIMEOUT);
    case DS18B20_STATUS_CRC:     return (SENSOR_ERROR_CRC);
    default:                     return (SENSOR_ERROR_BUS);
  }
}




// -------------------------------------------------------------
static void temperatureSensorService_ApplyAge(
  SensorSnapshot_TypeDef* snapshot
) {
  if ((snapshot->lastSuccess != 0U)
      && ((snapshot->health == SENSOR_HEALTH_HEALTHY)
        || (snapshot->health == SENSOR_HEALTH_DEGRADED))
      && ((xTaskGetTickCount() - snapshot->lastSuccess)
        > pdMS_TO_TICKS(SENSOR_STALE_PERIOD_MS))) {
    snapshot->health = SENSOR_HEALTH_STALE;
    snapshot->dataValid = pdFALSE;
  }
}




// -------------------------------------------------------------
static void temperatureSensorService_PrintMeasurements(
  const DS18B20_Measurement_TypeDef* ds18b20Measurements,
  uint8_t ds18b20Count,
  ErrorStatus bmx280Status,
  ErrorStatus bmx680Status
) {
  printf("DS18B20: ");
  if (ds18b20Count > 0U) {
    for (uint8_t i = 0U; i < ds18b20Count; i++) {
      if (i > 0U) printf(", ");
      if (ds18b20Measurements[i].status == DS18B20_STATUS_OK) {
        temperatureSensorService_PrintCentiDegrees(
          ds18b20Measurements[i].temperature
        );
      } else {
        printf("error");
      }
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
