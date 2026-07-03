/**
  ******************************************************************************
  * @file           : eth_loopback.c
  *                   This file contains the ethernet loopback service code.
  ******************************************************************************
  * @attention
  *
  ******************************************************************************
  */
 

/* Includes ------------------------------------------------------------------*/
#include "eth_loopback.h"

/* Global variables ----------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static void ethLoopbackTask(void* parameters);

/**
  * @brief  Heartbeat LED blinking
  * @param  port: pointer to the GPIO port instance
  * @param  pin:  pin number (0..15)
  * @param  callbackDelay:  pointer to delay function 
  * @param  delay:  delay value 
  * @retval none
  */
static void ethLoopback(void); 


static uint8_t loopback_buf[128];
static uint8_t loopback_buf2[128];



/*******************************************************************************/

void EthLoopbackService(void) {

  static StaticTask_t ethLoopbackTaskTCB;
  static StackType_t ethLoopbackTaskStack[512];

  (void) xTaskCreateStatic(
                            ethLoopbackTask,
                            "Eth Loopback",
                            512,
                            NULL,
                            configMAX_PRIORITIES - 3U,
                            &(ethLoopbackTaskStack[0]),
                            &(ethLoopbackTaskTCB)
                          );
}



static void ethLoopbackTask(void* parameters) {
  /* Unused parameters. */
  (void) parameters;

  while(1) {
      ethLoopback();
  }
}




static void ethLoopback(void) {
  // vTaskDelay(10);
  SPI_Enable(SPI1);
  loopback_tcps(0, loopback_buf, 5300);
  loopback_tcps(1, loopback_buf2, 5301);
  SPI_Disable(SPI1);
}


