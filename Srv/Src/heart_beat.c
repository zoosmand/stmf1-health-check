/**
  ******************************************************************************
  * @file           : heart_beat.c
  *                   This file contains the LED blinking code that represent 
  *                   the system health.
  ******************************************************************************
  * @attention
  *
  ******************************************************************************
  */
 

/* Includes ------------------------------------------------------------------*/
#include "heart_beat.h"

/* Global variables ----------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static void heartBeatTask(void* parameters);

/**
  * @brief  Heartbeat LED blinking
  * @param  port: pointer to the GPIO port instance
  * @param  pin:  pin number (0..15)
  * @param  callbackDelay:  pointer to delay function 
  * @param  delay:  delay value 
  * @retval none
  */
static void heartBeat_Blink(GPIO_TypeDef*, uint16_t, void (*)(TickType_t), TickType_t); 




/*******************************************************************************/

void HeartBeatService(void) {

  static StaticTask_t heartBeatTaskTCB;
  static StackType_t heartBeatTaskStack[configMINIMAL_STACK_SIZE];

  (void) xTaskCreateStatic(
                            heartBeatTask,
                            "Heart Beat",
                            configMINIMAL_STACK_SIZE,
                            NULL,
                            configMAX_PRIORITIES - 1U,
                            &(heartBeatTaskStack[0]),
                            &(heartBeatTaskTCB)
                          );
}



static void heartBeatTask(void* parameters) {
  /* Unused parameters. */
  (void) parameters;

  while(1) {
      heartBeat_Blink(GPIOC, GPIO_PIN_13, vTaskDelay, 1200);
  }
}




static void heartBeat_Blink(GPIO_TypeDef* port, uint16_t pin, void (*callbackDelay)(TickType_t), TickType_t delay) {
  TickType_t fraction = delay/10;

  PIN_L(port, pin);
  callbackDelay(fraction);
  
  PIN_H(port, pin);
  callbackDelay(fraction);
  
  PIN_L(port, pin);
  callbackDelay(fraction);
  
  PIN_H(port, pin);
  callbackDelay(delay - fraction);
}


