/**
  ******************************************************************************
  * @file           : ds18b20.h
  * @brief          : Header for ds18b20.c file.
  *                   This file contains the common defines for DS18B20
  *                   temperature measurment service.
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



/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  Temperature measurement service based on OneWire devices
  * @param  none
  * @retval none
 */
void TemperatureMeasurmentService(void);


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