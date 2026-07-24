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


/* Private function prototypes -----------------------------------------------*/
static void temperatureMeasurementTask(void* parameters);

/**
  * @brief  Temperature measurement workflow
  * @retval none
  */
static ErrorStatus temperatureMeasurement_Workflow(void); 

__STATIC_INLINE void dS18B20_Command(uint8_t);

static ErrorStatus dS18B20_ReadScratchpad(uint8_t*, uint8_t*);

static ErrorStatus dS18B20_ConvertTemperature(uint8_t*);

static ErrorStatus dS18B20_WaitStatus(uint16_t);

static ErrorStatus DS18B20_GetTemperatureMeasurment(OneWireDevice_t*);

static void dS18B20_PrintTemperature(const uint8_t*);

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
  if (OneWire_Lock(portMAX_DELAY) != pdTRUE) return (ERROR);

  uint8_t deviceCount = OneWire_GetDeviceCount();
  if (deviceCount == 0U) {
    OneWire_Unlock();
    return (ERROR);
  }

  OneWireDevice_t* devs = Get_OwDevices();
  uint8_t reportCount = (deviceCount < MAX_REPORTED_DEVICES) ? deviceCount : MAX_REPORTED_DEVICES;

  for (uint8_t i = 0; i < reportCount; i++) {
    if (DS18B20_GetTemperatureMeasurment(&devs[i])) {
      OneWire_Unlock();
      return (ERROR);
    }
  }

  for (uint8_t i = 0; i < reportCount; i++) {
    dS18B20_PrintTemperature(devs[i].spad);
    printf((i + 1U < reportCount) ? " " : "\n");
  }

  OneWire_Unlock();
  return (SUCCESS);
}




// -------------------------------------------------------------
static void dS18B20_PrintTemperature(const uint8_t* scratchpad) {
  int16_t raw = (int16_t)(((uint16_t)scratchpad[1] << 8) | scratchpad[0]);
  int32_t centiDegrees = ((int32_t)raw * 100) / 16;
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
