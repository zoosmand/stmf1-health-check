/**
  ******************************************************************************
  * @file           : spi.h
  * @brief          : Header for spi.c file.
  *                   This file contains the common defines for the SPI
  *                   initialization functions.
  ******************************************************************************
  * @attention
  *
  ******************************************************************************
  */
 
  /* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SPI_INIT_H
#define __SPI_INIT_H

#ifdef __cplusplus
  extern "C" {
#endif 

/* Includes ------------------------------------------------------------------*/
#include "main.h"


#define SPI1_SCK_Pin          GPIO_PIN_5
#define SPI1_MISO_Pin         GPIO_PIN_6
#define SPI1_MOSI_Pin         GPIO_PIN_7
#define SPI1_Port             GPIOA

#define ETH_CS_Port           GPIOA
#define ETH_CS_Pin            GPIO_PIN_4
#define ETH_RST_Port          GPIOA
#define ETH_RST_Pin           GPIO_PIN_3
#define ETH_INT_Port          GPIOA
#define ETH_INT_Pin           GPIO_PIN_1

#define SPI2_SCK_Pin          GPIO_PIN_13
#define SPI2_MISO_Pin         GPIO_PIN_14
#define SPI2_MOSI_Pin         GPIO_PIN_15
#define SPI2_Port             GPIOB

#define SPI_BUS_TMOUT         10000 /* cycles timeout on SPI bus operations */



/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  Initializes the corresponding SPI peripheral. 
  *         The pin is configured as:
  *           - output, push-pull
  *           - low speed (2 MHz)
  *           - no pull-up and no pull-down
  * @param  port: pointer to the SPI port instance
  * @retval (int) Status of operation (0 = success)
  */
ErrorStatus SPI_Init(SPI_TypeDef*);

ErrorStatus SPI_AdjustInit(SPI_TypeDef*);

/**
 * @brief  Enables the given SPI peripherals.
 * @param  SPIx: pointer to the given SPI peripherals
 * @retval status of operation
 */
ErrorStatus SPI_Enable(SPI_TypeDef*);

/**
 * @brief  Disables the given SPI peripherals.
 * @param  SPIx: pointer to the given SPI peripherals
 * @retval status of operation
 */
ErrorStatus SPI_Disable(SPI_TypeDef*);

/**
  * @brief  Reads data from SPI bus with 8-bit data buffer size.
  * @param  SPIx: pointer to the dedicatred SPI bus structure
  * @param  buf: pointer to the buffer to store the read data
  * @param  cnt: count of bytes to read from the bus
  * @retval status of aperation
  */
ErrorStatus SPI_Read_8b(SPI_TypeDef*, uint8_t*, uint16_t);

/**
  * @brief  Writes data to the SPI bus with 8-bit data buffer size.
  * @param  SPIx: pointer to the dedicatred SPI bus structure
  * @param  buf: pointer to the buffer of transmitting data
  * @param  cnt: count of bytes to write to the bus
  * @retval status of aperation
  */
ErrorStatus SPI_Write_8b(SPI_TypeDef*, uint8_t*, uint16_t);



#ifdef __cplusplus
}
#endif

#endif /* __SPI_INIT_H */


// #define HEARTBEAT_LED_Pin GPIO_PIN_13
// #define HEARTBEAT_LED_GPIO_Port GPIOC
// #define ETH_IN_Pin GPIO_PIN_2
// #define ETH_IN_GPIO_Port GPIOA
// #define ETH_RESET_Pin GPIO_PIN_3
// #define ETH_RESET_GPIO_Port GPIOA
// #define ETH_CS_Pin GPIO_PIN_4
// #define ETH_CS_GPIO_Port GPIOA

// /* USER CODE BEGIN Private defines */
// #define HEARTBEAT_LED_Pin_Pos 13U
// #define USART_OUT USART1
