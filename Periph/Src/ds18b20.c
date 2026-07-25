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
#include <string.h>


/* Global variables ----------------------------------------------------------*/

/* Private defines -----------------------------------------------------------*/
#define DS18B20_CONVERSION_TIMEOUT_MS 750U

/* Private function prototypes -----------------------------------------------*/
__STATIC_INLINE void dS18B20_Command(uint8_t);

static DS18B20_Status_TypeDef dS18B20_ReadScratchpad(uint8_t*, uint8_t*);

static DS18B20_Status_TypeDef dS18B20_ConvertTemperature(uint8_t*);

static ErrorStatus dS18B20_WaitStatus(uint16_t);

static DS18B20_Status_TypeDef DS18B20_GetTemperatureMeasurement(
  OneWireDevice_t*
);

static int16_t dS18B20_DecodeTemperature(const uint8_t*);

// -------------------------------------------------------------
ErrorStatus DS18B20_Measure(
  DS18B20_Measurement_TypeDef* measurements,
  uint8_t capacity,
  uint8_t* count
) {
  if ((measurements == NULL) || (count == NULL) || (capacity == 0U)) {
    return (ERROR);
  }
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
    memcpy(measurements[i].rom, devices[i].addr, sizeof(measurements[i].rom));
    measurements[i].status = DS18B20_GetTemperatureMeasurement(&devices[i]);
    if (measurements[i].status == DS18B20_STATUS_OK) {
      measurements[i].temperature = dS18B20_DecodeTemperature(devices[i].spad);
    } else {
      measurements[i].temperature = 0;
    }
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
__STATIC_INLINE void dS18B20_Command(uint8_t cmd) {
  OneWire_WriteByte(cmd);
}



// -------------------------------------------------------------  
static DS18B20_Status_TypeDef dS18B20_ReadScratchpad(
  uint8_t* buf,
  uint8_t* addr
) {

  if (OneWire_MatchROM(addr)) return (DS18B20_STATUS_BUS);
  dS18B20_Command(ReadScratchpad);

  uint8_t crc = 0;
  for (int8_t i = 0; i < 9; i++) {
    OneWire_ReadByte(&buf[i]);
    crc = OneWire_CRC8(crc, buf[i]);
  }
  if (crc) return (DS18B20_STATUS_CRC);
  
  return (DS18B20_STATUS_OK);
}




// -------------------------------------------------------------
static DS18B20_Status_TypeDef dS18B20_ConvertTemperature(uint8_t* addr) {

  if (*addr) {
    
    if (OneWire_MatchROM(addr)) return (DS18B20_STATUS_BUS);
    uint8_t pps = OneWire_ReadPowerSupply(addr);
    
    if (OneWire_MatchROM(addr)) return (DS18B20_STATUS_BUS);
    dS18B20_Command(ConvertT);
    
    if (pps) {
      OneWire_StrongPullupEnable();
      vTaskDelay(pdMS_TO_TICKS(DS18B20_CONVERSION_TIMEOUT_MS));
      OneWire_StrongPullupDisable();
    } else {
      if (dS18B20_WaitStatus(DS18B20_CONVERSION_TIMEOUT_MS) != SUCCESS) {
        return (DS18B20_STATUS_TIMEOUT);
      }
    }
  } else {
    if (OneWire_Reset()) return (DS18B20_STATUS_BUS);
    
    dS18B20_Command(SkipROM);
    dS18B20_Command(ConvertT);
    if (dS18B20_WaitStatus(DS18B20_CONVERSION_TIMEOUT_MS) != SUCCESS) {
      return (DS18B20_STATUS_TIMEOUT);
    }
  }

  return (DS18B20_STATUS_OK);
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
static DS18B20_Status_TypeDef DS18B20_GetTemperatureMeasurement(
  OneWireDevice_t* dev
) {
  DS18B20_Status_TypeDef status = dS18B20_ConvertTemperature(dev->addr);
  if (status != DS18B20_STATUS_OK) return (status);

  return dS18B20_ReadScratchpad(dev->spad, dev->addr);
}
