/**
  ******************************************************************************
  * @file           : i2c.c
  * @brief          : Polling I2C master-mode peripheral interface.
  ******************************************************************************
  */

#include "i2c.h"

#define I2C_BUS_TIMEOUT 100000U
#define I2C_APB1_HZ     36000000U
#define I2C_SPEED_HZ    100000U

static ErrorStatus i2c_WaitSet(I2C_TypeDef*, uint32_t);
static void i2c_Stop(I2C_TypeDef*);




// -------------------------------------------------------------
ErrorStatus I2C_Init(I2C_TypeDef* i2c) {
  if (i2c != I2C1) return (ERROR);

  uint32_t pinMode = GPIO_AF_OD | GPIO_IOS_2;
  MODIFY_REG(
    GPIOB->CRL,
    GPIO_PIN_6_Mask | GPIO_PIN_7_Mask,
    (pinMode << (I2C1_SCL_Pin * 4U)) | (pinMode << (I2C1_SDA_Pin * 4U))
  );
  PIN_H(I2C1_SCL_Port, I2C1_SCL_Pin);
  PIN_H(I2C1_SDA_Port, I2C1_SDA_Pin);

  CLEAR_BIT(i2c->CR1, I2C_CR1_PE);
  SET_BIT(RCC->APB1RSTR, RCC_APB1RSTR_I2C1RST);
  CLEAR_BIT(RCC->APB1RSTR, RCC_APB1RSTR_I2C1RST);

  i2c->CR2 = I2C_APB1_HZ / 1000000U;
  i2c->CCR = I2C_APB1_HZ / (2U * I2C_SPEED_HZ);
  i2c->TRISE = (I2C_APB1_HZ / 1000000U) + 1U;
  i2c->OAR1 = (1U << 14U);
  SET_BIT(i2c->CR1, I2C_CR1_PE);

  return (SUCCESS);
}




// -------------------------------------------------------------
ErrorStatus I2C_Master_Send(
  I2C_TypeDef* i2c,
  uint8_t slaveAddress,
  const uint8_t* buffer,
  uint16_t length
) {
  if ((i2c == NULL) || (buffer == NULL) || (length == 0U)) return (ERROR);

  uint32_t timeout = I2C_BUS_TIMEOUT;
  while ((READ_BIT(i2c->SR2, I2C_SR2_BUSY) != 0U) && (--timeout != 0U));
  if (timeout == 0U) return (ERROR);

  SET_BIT(i2c->CR1, I2C_CR1_START);
  if (i2c_WaitSet(i2c, I2C_SR1_SB) != SUCCESS) {
    i2c_Stop(i2c);
    return (ERROR);
  }

  i2c->DR = (uint8_t)(slaveAddress << 1U);
  if (i2c_WaitSet(i2c, I2C_SR1_ADDR) != SUCCESS) {
    i2c_Stop(i2c);
    return (ERROR);
  }
  (void)i2c->SR1;
  (void)i2c->SR2;

  for (uint16_t i = 0U; i < length; i++) {
    if (i2c_WaitSet(i2c, I2C_SR1_TXE) != SUCCESS) {
      i2c_Stop(i2c);
      return (ERROR);
    }
    i2c->DR = buffer[i];
  }

  if (i2c_WaitSet(i2c, I2C_SR1_BTF) != SUCCESS) {
    i2c_Stop(i2c);
    return (ERROR);
  }

  i2c_Stop(i2c);
  return (SUCCESS);
}




// -------------------------------------------------------------
static ErrorStatus i2c_WaitSet(I2C_TypeDef* i2c, uint32_t mask) {
  const uint32_t errorMask = I2C_SR1_BERR | I2C_SR1_ARLO | I2C_SR1_AF;
  uint32_t timeout = I2C_BUS_TIMEOUT;
  while (((i2c->SR1 & mask) == 0U)
      && ((i2c->SR1 & errorMask) == 0U)
      && (--timeout != 0U));
  if ((i2c->SR1 & errorMask) != 0U) {
    CLEAR_BIT(i2c->SR1, errorMask);
    return (ERROR);
  }
  return (timeout == 0U) ? ERROR : SUCCESS;
}




// -------------------------------------------------------------
static void i2c_Stop(I2C_TypeDef* i2c) {
  SET_BIT(i2c->CR1, I2C_CR1_STOP);
}
