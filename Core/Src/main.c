/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Global variables ----------------------------------------------------------*/
__IO uint32_t _PREG_ = 0;

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/




////////////////////////////////////////////////////////////////////////////////

/**
  * @brief  Main program entry point.
  * @param  None
  * @retval None
  */
int main(void) {

  /* Initialization of necessary peripherals */
  if (LED_Init(HEARTBEAT_PORT, HEARTBEAT_PIN) != SUCCESS) FLAG_SET(_PREG_, _PR_HEART_BEAT_LED);
  if (OneWire_Init(OneWire_PORT, OneWire_PIN) != SUCCESS) FLAG_SET(_PREG_, _PR_ONEWIRE_BUS);
  if (USART_Init(USART1) != SUCCESS) FLAG_SET(_PREG_, _PR_USART1_BUS);
  if ((SPI_Init(SPI1) != SUCCESS)
      || (EthSPI_Init(ETH_CS_Port, ETH_CS_Pin) != SUCCESS)
      || (EthSPI_Init(ETH_RST_Port, ETH_RST_Pin) != SUCCESS)
      || (SPI_AdjustInit(SPI1) != SUCCESS)) {
    FLAG_SET(_PREG_, _PR_SPI1_BUS);
  }
  #if defined(USE_WH_DISPLAY)
    if ((I2C_Init(I2C1) != SUCCESS)
        || (WHxxxx_Init(I2C1, WHxxxx_I2C_ADDR) != SUCCESS)) {
      FLAG_SET(_PREG_, _PR_I2C1_BUS);
    }
  #endif

  if (!FLAG_CHECK(_PREG_, _PR_SPI1_BUS) && (W5500_Init() != 0)) {
    FLAG_SET(_PREG_, _PR_SPI1_BUS);
  }

  printf("Peripherals readiness list: 0x%08lx\n", _PREG_);
  
  /* Run the Heartbeat Service */
  HeartBeatService();

  /* Run the Temperature Measurment Service */
  OneWireBusConfigurationInit();
  TemperatureMeasurmentService();

  /* TCP command service */
  if (!FLAG_CHECK(_PREG_, _PR_SPI1_BUS)) {
    TcpCommandService_Init();
  }


  /* Start the scheduler. */
  vTaskStartScheduler();

  while (1);
}



#if (configCHECK_FOR_STACK_OVERFLOW > 0)

    void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName) {
        /* Check pcTaskName for the name of the offending task,
         * or pxCurrentTCB if pcTaskName has itself been corrupted. */
        (void) xTask;
        (void) pcTaskName;
    }

#endif /* #if (configCHECK_FOR_STACK_OVERFLOW > 0) */



















/**
  * @brief  Setup the microcontroller system
  *         Initialize the Embedded Flash Interface, the PLL and update the 
  *         SystemCoreClock variable.
  * @note   This function should be used only after reset.
  * @param  None
  * @retval None
  */
void SystemInit (void) {
  uint32_t timeout;

  #if (PREFETCH_ENABLE != 0)
    PREG_SET(FLASH->ACR, FLASH_ACR_PRFTBE_Pos);
  #else
    PREG_CLR(FLASH->ACR, FLASH_ACR_PRFTBE_Pos);
  #endif /* PREFETCH_ENABLE */

  __NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

  /* SysCfg */
  PREG_SET(RCC->APB2ENR, RCC_APB2ENR_AFIOEN_Pos);
  while (!(PREG_CHECK(RCC->APB2ENR, RCC_APB2ENR_AFIOEN_Pos)));

  /* PWR */
  PREG_SET(RCC->APB1ENR, RCC_APB1ENR_PWREN_Pos);
  while (!(PREG_CHECK(RCC->APB1ENR, RCC_APB1ENR_PWREN_Pos)));

  /* Flash */
  MODIFY_REG(FLASH->ACR, FLASH_ACR_LATENCY, FLASH_ACR_LATENCY_1);
  if (READ_BIT(FLASH->ACR, FLASH_ACR_LATENCY) != FLASH_ACR_LATENCY_1) {
    Error_Handler();
  }

  /* JTAG-DP disabled and SW-DP enabled */
  CLEAR_BIT(AFIO->MAPR, AFIO_MAPR_SWJ_CFG);
  SET_BIT(AFIO->MAPR, AFIO_MAPR_SWJ_CFG_JTAGDISABLE);

  /* HSE enable and wait until it runs */
  PREG_SET(RCC->CR, RCC_CR_HSEON_Pos);
  timeout = 72000000U;
  while (!(PREG_CHECK(RCC->CR, RCC_CR_HSERDY_Pos)) && (--timeout != 0U));
  if (timeout == 0U) Error_Handler();

  /* LSI enable and wait until it runs */
  PREG_SET(RCC->CSR, RCC_CSR_LSION_Pos);
  timeout = 72000000U;
  while (!(PREG_CHECK(RCC->CSR, RCC_CSR_LSIRDY_Pos)) && (--timeout != 0U));
  if (timeout == 0U) Error_Handler();

  /* Enable backup registers access */
  PREG_SET(PWR->CR, PWR_CR_DBP_Pos);

  /* Force backup domain reset */
  PREG_SET(RCC->BDCR, RCC_BDCR_BDRST_Pos);
  PREG_CLR(RCC->BDCR, RCC_BDCR_BDRST_Pos);

  /* LSE enable and wait until it runs */
  PREG_SET(RCC->BDCR, RCC_BDCR_LSEON_Pos);
  timeout = 360000000U;
  while (!(PREG_CHECK(RCC->BDCR, RCC_BDCR_LSERDY_Pos)) && (--timeout != 0U));
  if (timeout == 0U) Error_Handler();

  /* RTC Source is LSE */
  MODIFY_REG(RCC->BDCR, RCC_BDCR_RTCSEL, RCC_BDCR_RTCSEL_0);

  /* Enable RTC */
  PREG_SET(RCC->BDCR, RCC_BDCR_RTCEN_Pos);

  /* PLL confugure domain clock */
  RCC->CFGR |= RCC_CFGR_PLLMULL9; // mutiprexing pll on 9
  PREG_SET(RCC->CFGR, RCC_CFGR_PLLSRC_Pos); // PLL is the clock source

  /* PLL enable and wait until it runs */
  PREG_SET(RCC->CR, RCC_CR_PLLON_Pos);
  timeout = 72000000U;
  while (!(PREG_CHECK(RCC->CR, RCC_CR_PLLRDY_Pos)) && (--timeout != 0U));
  if (timeout == 0U) Error_Handler();

  /* AHB clock isn't divided */
  /* APB1 clock divided by 2 */
  MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE1, RCC_CFGR_PPRE1_DIV2);
  /* APB2 clock isn't divided */

  /* set PLL as sysclock source and wait until it runs */
  MODIFY_REG(RCC->CFGR, RCC_CFGR_SW, RCC_CFGR_SW_PLL);
  timeout = 72000000U;
  while ((READ_BIT(RCC->CFGR, RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) && (--timeout != 0U));
  if (timeout == 0U) Error_Handler();



  SET_BIT(CoreDebug->DEMCR, CoreDebug_DEMCR_TRCENA_Msk);


  /*----------------------------------------------------------------------------*/
  /*                          Set peripheral clocks                             */
  /*----------------------------------------------------------------------------*/
  /* AHB peripherals */
  SET_BIT(RCC->AHBENR, (
      RCC_AHBENR_DMA1EN
    | RCC_AHBENR_CRCEN
    | RCC_AHBENR_SRAMEN
  ));

  /* APB1 peripherals */
  #if defined(USE_WH_DISPLAY)
    SET_BIT(RCC->APB1ENR, RCC_APB1ENR_I2C1EN);
  #endif

  /* APB2 peripherals */
  SET_BIT(RCC->APB2ENR, (
      RCC_APB2ENR_IOPAEN
    | RCC_APB2ENR_IOPBEN
    | RCC_APB2ENR_IOPCEN
    | RCC_APB2ENR_USART1EN
    | RCC_APB2ENR_SPI1EN
  ));


  
  
  /* Stop ticking peripheral while debugging */
  #ifdef DEBUG
    SET_BIT(DBGMCU->CR , (
        DBGMCU_CR_DBG_IWDG_STOP
      | DBGMCU_CR_DBG_WWDG_STOP
    ));
  #endif /* DEBUG */



}
