/**
  ******************************************************************************
  * @file           : onewire.h
  * @brief          : Header for onewire.c file.
  *                   This file contains the common defines for OneWire 
  *                   devices.
  ******************************************************************************
  * @attention
  *
  ******************************************************************************
  */
 
  /* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __ONEWIRE_H
#define __ONEWIRE_H

#ifdef __cplusplus
  extern "C" {
#endif 

/* Includes ------------------------------------------------------------------*/
#include "main.h"


/* Private typedef -----------------------------------------------------------*/
typedef struct {
  uint8_t   addr[8];
  uint8_t   spad[9];
} OneWireDevice_t;


/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

ErrorStatus OneWire_Reset(void);

void OneWire_WriteByte(uint8_t);

uint8_t OneWire_ReadBit(void);

void OneWire_ReadByte(uint8_t*);

uint8_t OneWire_CRC8(uint8_t, uint8_t);

ErrorStatus OneWire_Search(void);

void OneWireBusConfigurationInit(void);

BaseType_t OneWire_Lock(TickType_t);

void OneWire_Unlock(void);

uint8_t OneWire_GetDeviceCount(void);

/**
 * @brief   Defines parasitic powered devices on OnWire bus.
 * @param   addr pointer to OneWire device address
 * @retval  (uint8_t) status of power supply
 */
uint8_t OneWire_ReadPowerSupply(uint8_t*);

/**
 * @brief   Determines the existent of the device with given address, on the bus.
 * @param   addr pointer to OneWire device address
 * @retval  (uint8_t) status of operation
 */
ErrorStatus OneWire_MatchROM(uint8_t*);


OneWireDevice_t* Get_OwDevices(void);


/* Exported defines -----------------------------------------------------------*/
#define SearchROM       0xf0
#define ReadROM         0x33
#define MatchROM        0x55
#define SkipROM         0xcc
#define ReadPowerSupply 0xb4

#define OneWire_PORT    GPIOB
#define OneWire_PIN     GPIO_PIN_9_Pos

#define OneWire_Low     PIN_H(OneWire_PORT, OneWire_PIN)
#define OneWire_High    PIN_L(OneWire_PORT, OneWire_PIN)
#define OneWire_Level   (PIN_LEVEL(OneWire_PORT, OneWire_PIN))



#ifdef __cplusplus
}
#endif

#endif /* __ONEWIRE_H */
