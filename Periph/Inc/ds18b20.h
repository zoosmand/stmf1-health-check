/**
  ******************************************************************************
  * @file           : ds18b20.h
  * @brief          : DS18B20 temperature sensor interface.
  ******************************************************************************
  * @attention
  *
  ******************************************************************************
  */
 
  /* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __DS18B20_H
#define __DS18B20_H

#ifdef __cplusplus
  extern "C" {
#endif 

/* Includes ------------------------------------------------------------------*/
#include "main.h"



typedef enum {
  DS18B20_STATUS_OK = 0U,
  DS18B20_STATUS_BUS,
  DS18B20_STATUS_TIMEOUT,
  DS18B20_STATUS_CRC
} DS18B20_Status_TypeDef;

typedef struct {
  uint8_t rom[8];
  int16_t temperature;
  DS18B20_Status_TypeDef status;
} DS18B20_Measurement_TypeDef;

/**
  * @brief Converts and reads every discovered DS18B20 independently.
  * @param measurements Per-device identity, value, and status output.
  * @param capacity Number of elements available in measurements.
  * @param count Number of discovered devices written.
  * @retval SUCCESS when at least one discovered device was reported.
  */
ErrorStatus DS18B20_Measure(
  DS18B20_Measurement_TypeDef*,
  uint8_t,
  uint8_t*
);


/* Private defines -----------------------------------------------------------*/
#define AlarmSearch     0xec
#define ConvertT        0x44
#define WriteScratchpad 0x4e
#define ReadScratchpad  0xbe
#define CopyScratchpad  0x48
#define RecallE         0xb8
#define ReadPowerSupply 0xb4



#ifdef __cplusplus
}
#endif

#endif /* __DS18B20_H */
