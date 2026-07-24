/**
  ******************************************************************************
  * @file           : ds18b20.c
  *                   This file contains OneWire DS18B20 temperature sensor
  *                   code. 
  ******************************************************************************
  * @attention
  *
  ******************************************************************************
  */
 

/* Includes ------------------------------------------------------------------*/
#include "ds18b20.h"


/* Global variables ----------------------------------------------------------*/

/* Private defines -----------------------------------------------------------*/
#define MAX_REPORTED_DEVICES          2U
#define DS18B20_CONVERSION_TIMEOUT_MS 750U

/* Private variables ---------------------------------------------------------*/
static int16_t recentTemperatures[MAX_REPORTED_DEVICES];
static uint8_t recentTemperatureCount;
static BaseType_t recentTemperaturesValid;

/* Private function prototypes -----------------------------------------------*/
static void temperatureMeasurementTask(void* parameters);

/**
  * @brief  Temperature measurement workflow
  * @retval none
  */
static ErrorStatus temperatureMeasurement_Workflow(void);

static ErrorStatus dS18B20_MeasureTemperatures(int16_t*, uint8_t, uint8_t*);

__STATIC_INLINE void dS18B20_Command(uint8_t);

static ErrorStatus dS18B20_ReadScratchpad(uint8_t*, uint8_t*);

static ErrorStatus dS18B20_ConvertTemperature(uint8_t*);

static ErrorStatus dS18B20_WaitStatus(uint16_t);

static ErrorStatus DS18B20_GetTemperatureMeasurment(OneWireDevice_t*);

static int16_t dS18B20_DecodeTemperature(const uint8_t*);

static void dS18B20_PrintTemperature(int16_t);

/*******************************************************************************/

// -------------------------------------------------------------  
void TemperatureMeasurmentService(void) {
  static StaticTask_t temperatureMeasurementTaskTCB;
  static StackType_t temperatureMeasurementTaskStack[configMINIMAL_STACK_SIZE * 4];
  
  (void) xTaskCreateStatic(
    temperatureMeasurementTask,
    "Temp Meas",
    configMINIMAL_STACK_SIZE * 4,
    NULL,
    configMAX_PRIORITIES - 2,
    &(temperatureMeasurementTaskStack[0]),
    &(temperatureMeasurementTaskTCB)
  );
}




// -------------------------------------------------------------  
static void temperatureMeasurementTask(void* parameters) {
  /* Unused parameters. */
  (void) parameters;
  
  while(1) {
    (void) temperatureMeasurement_Workflow();
    vTaskDelay(4000);
  }
}




// -------------------------------------------------------------  
static ErrorStatus temperatureMeasurement_Workflow(void) {
  int16_t temperatures[MAX_REPORTED_DEVICES];
  uint8_t count = 0;

  if (dS18B20_MeasureTemperatures(temperatures, MAX_REPORTED_DEVICES, &count) != SUCCESS) {
    return (ERROR);
  }

  taskENTER_CRITICAL();
  for (uint8_t i = 0U; i < count; i++) {
    recentTemperatures[i] = temperatures[i];
  }
  recentTemperatureCount = count;
  recentTemperaturesValid = pdTRUE;
  taskEXIT_CRITICAL();

  for (uint8_t i = 0; i < count; i++) {
    dS18B20_PrintTemperature(temperatures[i]);
    printf((i + 1U < count) ? " " : "\n");
  }
  return (SUCCESS);
}




// -------------------------------------------------------------
ErrorStatus DS18B20_GetRecentTemperatures(int16_t* temperatures, uint8_t capacity, uint8_t* count) {
  if ((temperatures == NULL) || (count == NULL) || (capacity == 0U)) return (ERROR);
  *count = 0U;

  taskENTER_CRITICAL();
  if (recentTemperaturesValid == pdFALSE) {
    taskEXIT_CRITICAL();
    return (ERROR);
  }

  uint8_t copyCount = (recentTemperatureCount < capacity) ? recentTemperatureCount : capacity;
  for (uint8_t i = 0U; i < copyCount; i++) {
    temperatures[i] = recentTemperatures[i];
  }
  *count = copyCount;
  taskEXIT_CRITICAL();

  return (copyCount > 0U) ? SUCCESS : ERROR;
}




// -------------------------------------------------------------
static ErrorStatus dS18B20_MeasureTemperatures(
  int16_t* temperatures,
  uint8_t capacity,
  uint8_t* count
) {
  *count = 0U;
  if (OneWire_Lock(portMAX_DELAY) != pdTRUE) return (ERROR);

  uint8_t deviceCount = OneWire_GetDeviceCount();
  uint8_t readCount = (deviceCount < capacity) ? deviceCount : capacity;
  if (readCount == 0U) {
    OneWire_Unlock();
    return (ERROR);
  }

  OneWireDevice_t* devices = Get_OwDevices();
  for (uint8_t i = 0; i < readCount; i++) {
    if (DS18B20_GetTemperatureMeasurment(&devices[i]) != SUCCESS) {
      OneWire_Unlock();
      return (ERROR);
    }
    temperatures[i] = dS18B20_DecodeTemperature(devices[i].spad);
  }

  *count = readCount;
  OneWire_Unlock();
  return (SUCCESS);
}




// -------------------------------------------------------------
static int16_t dS18B20_DecodeTemperature(const uint8_t* scratchpad) {
  int16_t raw = (int16_t)(((uint16_t)scratchpad[1] << 8) | scratchpad[0]);
  return (int16_t)(((int32_t)raw * 100) / 16);
}




// -------------------------------------------------------------
static void dS18B20_PrintTemperature(int16_t centiDegrees) {
  uint32_t magnitude = (centiDegrees < 0) ? (uint32_t)(-centiDegrees) : (uint32_t)centiDegrees;

  if (centiDegrees < 0) printf("-");
  printf("%lu.%02lu", (unsigned long)(magnitude / 100U), (unsigned long)(magnitude % 100U));
}




/*******************************************************************************/

// -------------------------------------------------------------  
__STATIC_INLINE void dS18B20_Command(uint8_t cmd) {
  OneWire_WriteByte(cmd);
}



// -------------------------------------------------------------  
static ErrorStatus dS18B20_ReadScratchpad(uint8_t* buf, uint8_t* addr) {

  if (OneWire_MatchROM(addr)) return (ERROR);
  dS18B20_Command(ReadScratchpad);

  uint8_t crc = 0;
  for (int8_t i = 0; i < 9; i++) {
    OneWire_ReadByte(&buf[i]);
    crc = OneWire_CRC8(crc, buf[i]);
  }
  if (crc) return (ERROR);
  
  return (SUCCESS);
}




// -------------------------------------------------------------
static ErrorStatus dS18B20_ConvertTemperature(uint8_t* addr) {

  if (*addr) {
    
    if (OneWire_MatchROM(addr)) return (ERROR);
    uint8_t pps = OneWire_ReadPowerSupply(addr);
    
    if (OneWire_MatchROM(addr)) return (ERROR);
    dS18B20_Command(ConvertT);
    
    if (pps) {
      OneWire_StrongPullupEnable();
      vTaskDelay(pdMS_TO_TICKS(DS18B20_CONVERSION_TIMEOUT_MS));
      OneWire_StrongPullupDisable();
    } else {
      if (dS18B20_WaitStatus(DS18B20_CONVERSION_TIMEOUT_MS) != SUCCESS) return (ERROR);
    }
  } else {
    if (OneWire_Reset()) return (ERROR);
    
    dS18B20_Command(SkipROM);
    dS18B20_Command(ConvertT);
    if (dS18B20_WaitStatus(DS18B20_CONVERSION_TIMEOUT_MS) != SUCCESS) return (ERROR);
  }

  return (SUCCESS);
}




// -------------------------------------------------------------
static ErrorStatus dS18B20_WaitStatus(uint16_t timeoutMs) {
  uint32_t attempts = ((uint32_t)timeoutMs * 1000U) / 70U;

  while (attempts-- > 0U) {
    if (OneWire_ReadBit()) return (SUCCESS);
  }
  return (ERROR);
}




// -------------------------------------------------------------  
// -------------------------------------------------------------  
static ErrorStatus DS18B20_GetTemperatureMeasurment(OneWireDevice_t *dev) {

  if (dS18B20_ConvertTemperature(dev->addr)) return (ERROR);
  if (dS18B20_ReadScratchpad(dev->spad, dev->addr)) return (ERROR);

  return (SUCCESS);
}
