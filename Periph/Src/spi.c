/**
  ******************************************************************************
  * @file           : spi.c
  * @brief          : This file contains the common defines for the SPI 
  *                   initialization functions.
  ******************************************************************************
  * @attention
  *
  ******************************************************************************
  */
 

  /* Includes ------------------------------------------------------------------*/
#include "spi.h"

/* Global variables ---------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/


ErrorStatus SPI_Init(SPI_TypeDef* SPIx) {

  /* Enable GPIO SCK, MISO, MOSI alternative on high speed */

  if (SPIx == SPI1) {

    MODIFY_REG(SPI1_Port->CRL,
      ((0xf << (SPI1_SCK_Pin * 4U)) | (0xf << (SPI1_MISO_Pin * 4U)) | (0xf << (SPI1_MOSI_Pin * 4U))), (
        ((GPIO_AF_PP | GPIO_IOS_50) << (SPI1_SCK_Pin * 4U))
      | (GPIO_IN_FL << (SPI1_MISO_Pin * 4U))
      | ((GPIO_AF_PP | GPIO_IOS_50) << (SPI1_MOSI_Pin * 4U))
    ));
    /* Enbale SPI master mode */
    SET_BIT(SPIx->CR1, SPI_CR1_MSTR);
  }

  if (SPIx == SPI2) {
    MODIFY_REG(SPI2_Port->CRH,
      ((0xf << ((SPI2_SCK_Pin - 8) * 4U)) | (0xf << ((SPI2_MISO_Pin - 8) * 4U)) | (0xf << ((SPI2_MOSI_Pin - 8) * 4U))), (
        ((GPIO_AF_PP | GPIO_IOS_50) << ((SPI2_SCK_Pin - 8) * 4U))
      | (GPIO_IN_FL << ((SPI2_MISO_Pin - 8) * 4U))
      | ((GPIO_AF_PP | GPIO_IOS_50) << ((SPI2_MOSI_Pin - 8) * 4U))
    ));
    /* Enbale SPI master mode */
    SET_BIT(SPIx->CR1, SPI_CR1_MSTR);
  }

  switch ((uint32_t)SPIx) {
  case (uint32_t)SPI1:
    // NVIC_SetPriority(SPI1_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 15, 0));
    // NVIC_EnableIRQ(SPI1_IRQn);
    break;

    case (uint32_t)SPI2:
    // NVIC_SetPriority(SPI2_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 15, 0));
    // NVIC_EnableIRQ(SPI2_IRQn);
    break;
  
  default:
    return (ERROR);
    break;
  }

  return (SUCCESS);
}



// ----------------------------------------------------------------------------

ErrorStatus SPI_AdjustInit(SPI_TypeDef* SPIx) {

  MODIFY_REG(SPIx->CR1, (SPI_CR1_BR_Msk | SPI_CR1_DFF_Msk), 0);
  PREG_SET(SPIx->CR2, SPI_CR2_SSOE_Pos);

  return (SUCCESS);
}



// ----------------------------------------------------------------------------

ErrorStatus SPI_Enable(SPI_TypeDef* SPIx) {

  uint32_t tmout = SPI_BUS_TMOUT;

  while(PREG_CHECK(SPIx->SR, SPI_SR_BSY_Pos)) {
    if (!(--tmout)) {
      SPI_Disable(SPIx);
      return (ERROR);
    }
  }

  PREG_SET(SPIx->CR1, SPI_CR1_SPE_Pos);
  return (SUCCESS);
}




// ----------------------------------------------------------------------------

ErrorStatus SPI_Disable(SPI_TypeDef* SPIx) {
  uint32_t tmout = SPI_BUS_TMOUT;
  
  while(PREG_CHECK(SPIx->SR, SPI_SR_BSY_Pos)) {
    if (!(--tmout)) {
      return (ERROR);
    }
  }
  
  PREG_CLR(SPIx->CR1, SPI_CR1_SPE_Pos);
  return (SUCCESS);
}




// ----------------------------------------------------------------------------

ErrorStatus SPI_Read_8b(SPI_TypeDef* SPIx, uint8_t *buf, uint16_t cnt) {
  uint32_t tmout = 0;
  
  while (cnt--) {
    *(__IO uint8_t*)&SPIx->DR = 0;
    
    tmout = SPI_BUS_TMOUT;
    while(!(PREG_CHECK(SPIx->SR, SPI_SR_TXE_Pos))) {
      if (!(--tmout)) return (ERROR);
    }
    
    tmout = SPI_BUS_TMOUT;
    while(!(PREG_CHECK(SPIx->SR, SPI_SR_RXNE_Pos))) {
      if (!(--tmout)) return (ERROR);
    }
    *buf++ = (uint8_t)SPIx->DR;
  }
  
  return (SUCCESS);
}



// ----------------------------------------------------------------------------

ErrorStatus SPI_Write_8b(SPI_TypeDef* SPIx, uint8_t *buf, uint16_t cnt) {
  uint32_t tmout = 0;
  
  while (cnt--) {
    *(__IO uint8_t*)&SPIx->DR = *buf++;
    
    tmout = SPI_BUS_TMOUT;
    while(!(PREG_CHECK(SPIx->SR, SPI_SR_TXE_Pos))) {
      if (!(--tmout)) return (ERROR);
    }
    
    tmout = SPI_BUS_TMOUT;
    while(!(PREG_CHECK(SPIx->SR, SPI_SR_RXNE_Pos))) {
      if (!(--tmout)) return (ERROR);
    }
    (SPIx->DR);
  }
  
  return (SUCCESS);
}
