/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <timers.h>
#include <semphr.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
// #include <unistd.h>

#include "misc.h"
#include "stm32f103xb.h"

/* Private includes ----------------------------------------------------------*/
#include "common.h"
#include "stm32f10x_it.h"
#include "project_config.h"

#include "gpio.h"
#include "usart.h"
#include "onewire.h"
#include "spi.h"
#include "i2c.h"
#include "whxxxx.h"
#include "ssd13xx.h"
#include "bmx280.h"
#include "bmx680.h"

#include "heart_beat.h"
#include "temperature_service.h"
#include "tcp_service.h"

#include "wizchip_port.h"


/* Exported constants --------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/
extern __IO uint32_t _PREG_;

/* Private defines -----------------------------------------------------------*/
/* Peripherals readiness flags */
#define _PR_HEART_BEAT_LED    0
#define _PR_USART1_BUS        1
#define _PR_ONEWIRE_BUS       2
#define _PR_SPI1_BUS          3
#define _PR_I2C1_BUS          4
#define _PR_WH_DISPLAY        5
#define _PR_BMX280            6
#define _PR_BMX680            7
#define _PR_SSD_DISPLAY       8

/* Exported functions prototypes ---------------------------------------------*/


/* Service functions related to the CM3 BitBand */

/**
 * 
 */
extern void Set_BitBandVal(uint32_t, uint32_t);

/**
 * 
 */
extern uint32_t Get_BitBandVal(uint32_t);

/**
 * 
 */
void _delay_us(uint32_t);
void _delay_ms(uint32_t);


#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
