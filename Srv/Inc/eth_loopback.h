/**
  ******************************************************************************
  * @file           : eth_loopback.h
  * @brief          : Header for eth_loopback.c file.
  *                   This file contains the common defines for the ethernet
  *                   loopback service.
  ******************************************************************************
  * @attention
  *
  ******************************************************************************
  */
 
  /* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __ETH_LOOPBACK_H
#define __ETH_LOOPBACK_H

#ifdef __cplusplus
  extern "C" {
#endif 

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "loopback/loopback.h"



/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  Heartbeat LED blinking service
  * @param  none
  * @retval none
 */
void EthLoopbackService(void);


#ifdef __cplusplus
}
#endif

#endif /* __ETH_LOOPBACK_H */