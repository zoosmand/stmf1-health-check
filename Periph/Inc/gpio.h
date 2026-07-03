/**
  ******************************************************************************
  * @file           : gpio.h
  * @brief          : Header for gpio.c file.
  *                   This file contains the common defines for the GPIO
  *                   initialization functions.
  ******************************************************************************
  * @attention
  *
  ******************************************************************************
  */
 
  /* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __GPIO_INIT_H
#define __GPIO_INIT_H

#ifdef __cplusplus
  extern "C" {
#endif 

/* Includes ------------------------------------------------------------------*/
#include "main.h"


/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  Initializes the corresponding LED pin on the board. 
  *         The pin is configured as:
  *           - output
  *           - low speed (2 MHz)
  *           - push-pull
  * @param  port: pointer to the GPIO port instance
  * @param  pin:  pin number (0..15)
  * @retval (int) Status of operation (0 = success)
  */
ErrorStatus LED_Init(GPIO_TypeDef*, uint16_t);


/**
  * @brief  Initializes the corresponding OneWire bus pin on the board. 
  *         The pin is configured as:
  *           - output
  *           - low speed (10 MHz)
  *           - open-drain
  * @param  port: pointer to the GPIO port instance
  * @param  pin:  pin number (0..15)
  * @retval (int) Status of operation (0 = success)
  */
ErrorStatus OneWire_Init(GPIO_TypeDef*, uint16_t);


/**
  * @brief  Initializes the corresponding Ethernet pins on the board. 
  *         The pin is configured as:
  *           - output
  *           - low speed (10 MHz)
  *           - open-drain
  * @param  port: pointer to the GPIO port instance
  * @param  pin:  pin number (0..15)
  * @retval (int) Status of operation (0 = success)
  */
ErrorStatus EthSPI_Init(GPIO_TypeDef*, uint16_t);


#ifdef __cplusplus
}
#endif

#endif /* __GPIO_INIT_H */