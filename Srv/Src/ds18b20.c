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

/* Private variables ---------------------------------------------------------*/
static SemaphoreHandle_t gOwMutex;


/* Private defines -----------------------------------------------------------*/
#define _delay_ms vTaskDelay

/* Private function prototypes -----------------------------------------------*/
static void temperatureMeasurementTask(void* parameters);

/**
  * @brief  Temperature measurement workflow
  * @retval none
  */
static int temperatureMeasurement_Workflow(void); 

__STATIC_INLINE void dS18B20_Command(uint8_t);

__STATIC_INLINE void dS18B20_Write(uint8_t, uint8_t*);

static int dS18B20_Read(uint8_t, uint8_t*, uint8_t);

static int dS18B20_ReadScratchpad(uint8_t*, uint8_t*);

static int DS18B20_CopyScratchpad(uint8_t*);

static void dS18B20_ErrorHandler(void);

static int dS18B20_ConvertTemperature(uint8_t*);

__STATIC_INLINE void dS18B20_WaitStatus(uint16_t);

static int DS18B20_GetTemperatureMeasurment(OneWireDevice_t*);





// -------------------------------------------------------------
__STATIC_INLINE BaseType_t ow_lock(TickType_t to) {
  return xSemaphoreTake(gOwMutex, to);
}



// -------------------------------------------------------------
__STATIC_INLINE void ow_unlock(void) {
  xSemaphoreGive(gOwMutex);
}




/*******************************************************************************/

// -------------------------------------------------------------  
void TemperatureMeasurmentService(void) {
  
  gOwMutex = xSemaphoreCreateMutex();
  static StaticTask_t temperatureMeasurementTaskTCB;
  static StackType_t temperatureMeasurementTaskStack[configMINIMAL_STACK_SIZE * 4];
  
  (void) xTaskCreateStatic(
    temperatureMeasurementTask,
    "Temp Meas",
    configMINIMAL_STACK_SIZE * 4,
    NULL,
    tskIDLE_PRIORITY + 1,
    &(temperatureMeasurementTaskStack[0]),
    &(temperatureMeasurementTaskTCB)
  );
}




// -------------------------------------------------------------  
static void temperatureMeasurementTask(void* parameters) {
  /* Unused parameters. */
  (void) parameters;
  
  while(1) {
    if (temperatureMeasurement_Workflow()) {
      vTaskDelete(NULL);
    }
    vTaskDelay(4000);
  }
}




// -------------------------------------------------------------  
static int temperatureMeasurement_Workflow(void) {
  if (OneWire_Reset()) return (1);

  OneWireDevice_t* devs = Get_OwDevices();

  for (uint8_t i = 0; i < 2; i++) {
    if (DS18B20_GetTemperatureMeasurment(&devs[i])) {
      devs[i].spad[0] = 0x00;
      devs[i].spad[1] = 0x08;
    }
  }

  uint32_t* t1 = (int32_t*)&devs[0].spad;
  uint32_t* t2 = (int32_t*)&devs[1].spad;
  printf("%d.%02d %d.%02d\n", 
    (int8_t)((*t1 & 0x0000fff0) >> 4), (uint8_t)(((*t1 & 0x0000000f) * 100) >> 4),
    (int8_t)((*t2 & 0x0000fff0) >> 4), (uint8_t)(((*t2 & 0x0000000f) * 100) >> 4)
  );
  return (0);
}




/*******************************************************************************/

// -------------------------------------------------------------  
__STATIC_INLINE void dS18B20_Command(uint8_t cmd) {
  OneWire_WriteByte(cmd);
}



// -------------------------------------------------------------  
__STATIC_INLINE void dS18B20_Write(uint8_t len, uint8_t* buf) {
  for (int i = 0; i < len; i++){
    OneWire_WriteByte(buf[i]);
  }
}



// -------------------------------------------------------------  
static int dS18B20_Read(uint8_t len, uint8_t* buf, uint8_t reverse) {
  uint8_t crc = 0;

  if (reverse) {
    for (int i = (len - 1); i >= 0; i--){
      OneWire_ReadByte(&buf[i]);
      crc = OneWire_CRC8(crc, buf[i]);
    }
  } else {
    for (int i = 0; i < len; i++){
      OneWire_ReadByte(&buf[i]);
      crc = OneWire_CRC8(crc, buf[i]);
    }
  }

  return crc;
}




// -------------------------------------------------------------  
static int dS18B20_ReadScratchpad(uint8_t* buf, uint8_t* addr) {

  // if (ow_lock(2000) != pdPASS) return -1;

  if (OneWire_MatchROM(addr)) return (1);
  dS18B20_Command(ReadScratchpad);

  uint8_t crc = 0;
  for (int8_t i = 0; i < 9; i++) {
    OneWire_ReadByte(&buf[i]);
    crc = OneWire_CRC8(crc, buf[i]);
  }
  if (crc) return (1);
  
  // ow_unlock();
  return (0);
}




// -------------------------------------------------------------
static int dS18B20_ConvertTemperature(uint8_t* addr) {

  // if (ow_lock(2000) != pdPASS) return -1;
  
  if (*addr) {
    
    if (OneWire_MatchROM(addr)) return (1);
    uint8_t pps = OneWire_ReadPowerSupply(addr);
    
    if (OneWire_MatchROM(addr)) return (1);
    dS18B20_Command(ConvertT);
    
    if (pps) {
      PIN_H(OneWire_PORT, OneWire_PIN);
      vTaskDelay(750);
      PIN_L(OneWire_PORT, OneWire_PIN);
    } else {
      dS18B20_WaitStatus(3);  
    }
  } else {
    if (OneWire_Reset()) return (1);
    
    dS18B20_Command(SkipROM);
    dS18B20_Command(ConvertT);
    dS18B20_WaitStatus(3);
  }

  // ow_unlock();
  return (0);
}




// -------------------------------------------------------------
static int DS18B20_CopyScratchpad(uint8_t* addr) {

  if (OneWire_MatchROM(addr)) return (1);
  uint8_t pps = OneWire_ReadPowerSupply(addr);

  if (OneWire_MatchROM(addr)) return (1);
  OneWire_WriteByte(CopyScratchpad);

  if (pps) {
    PIN_H(OneWire_PORT, OneWire_PIN);
    vTaskDelay(2);
    PIN_L(OneWire_PORT, OneWire_PIN);
  } else {
    dS18B20_WaitStatus(3);
  }
  return (0);
}




// -------------------------------------------------------------  
__STATIC_INLINE void dS18B20_WaitStatus(uint16_t ms) {
  while(!OneWire_ReadBit()) {
    __asm volatile("nop");
  };
}




// -------------------------------------------------------------  
static void dS18B20_ErrorHandler(void) {
  while (1) {
    /* code */
  }
}



// -------------------------------------------------------------  
static int DS18B20_GetTemperatureMeasurment(OneWireDevice_t *dev) {

  if (dS18B20_ConvertTemperature(dev->addr)) return (1);
  if (dS18B20_ReadScratchpad(dev->spad, dev->addr)) return (1);
  dS18B20_WaitStatus(3);

  return (0);
}