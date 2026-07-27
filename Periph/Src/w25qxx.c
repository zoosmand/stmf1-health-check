/**
  ******************************************************************************
  * @file           : w25qxx.c
  * @brief          : Winbond W25Qxx SPI NOR flash implementation.
  * @project        : STM32F1 Health Check Device
  * @platform       : STMicroelectronics STM32F103C8
  * @created        : 27.07.2026 09:52:45 AM
  ******************************************************************************
  * @attention
  * @copyright  : 2017-2026, Dmitry Slobodchikov
  ******************************************************************************
  */

#include "w25qxx.h"
#include "common.h"
#include "spi.h"
#include <stddef.h>

#define W25QXX_COMMAND_WRITE_ENABLE          0x06U
#define W25QXX_COMMAND_READ_JEDEC_ID         0x9fU
#define W25QXX_COMMAND_READ_UNIQUE_ID        0x4bU
#define W25QXX_COMMAND_READ_STATUS_1         0x05U
#define W25QXX_COMMAND_PAGE_PROGRAM          0x02U
#define W25QXX_COMMAND_FAST_READ             0x0bU
#define W25QXX_COMMAND_ERASE_SECTOR          0x20U
#define W25QXX_COMMAND_ERASE_BLOCK_32K       0x52U
#define W25QXX_COMMAND_ERASE_BLOCK_64K       0xd8U

#define W25QXX_STATUS_BUSY                   0x01U
#define W25QXX_ADDRESS_LENGTH                3U
#define W25QXX_FAST_READ_DUMMY_LENGTH        1U
#define W25QXX_UNIQUE_ID_DUMMY_LENGTH        4U
#define W25QXX_DMA_TRANSFER_LIMIT           0xffffU
#define W25QXX_DMA_TIMEOUT                1000000U
#define W25QXX_BUSY_POLL_LIMIT            1000000U
#define W25QXX_SELF_TEST_SEED                 0xa5U
#define W25QXX_SELF_TEST_STEP                 0x25U

#define W25QXX_SELECT() \
  PIN_L(w25qxxDevice.chipSelectPort, w25qxxDevice.chipSelectPin)
#define W25QXX_RELEASE() \
  PIN_H(w25qxxDevice.chipSelectPort, w25qxxDevice.chipSelectPin)

static W25Qxx_TypeDef w25qxxDevice = {
  .jedecId = 0U,
  .uniqueId = {0U},
  .capacity = 0U,
  .blockCount = 0U,
  .spi = SPI2,
  .chipSelectPort = W25Q64_DEFAULT_CS_PORT,
  .chipSelectPin = W25Q64_DEFAULT_CS_PIN,
};

static ErrorStatus w25qxx_ChipSelectInit(void);
static ErrorStatus w25qxx_BeginTransaction(void);
static ErrorStatus w25qxx_EndTransaction(ErrorStatus status);
static uint32_t w25qxx_DecodeAddress(uint32_t address);
static ErrorStatus w25qxx_ValidateRange(uint32_t address, uint32_t length);
static ErrorStatus w25qxx_TransferDMA(
  const uint8_t* transmitBuffer,
  uint8_t* receiveBuffer,
  uint16_t length
);
static ErrorStatus w25qxx_SendHeader(
  uint8_t command,
  uint32_t address,
  uint8_t addressLength,
  uint8_t dummyLength
);
static ErrorStatus w25qxx_ReadCommand(
  uint8_t command,
  uint32_t address,
  uint8_t addressLength,
  uint8_t dummyLength,
  uint8_t* buffer,
  uint32_t length
);
static ErrorStatus w25qxx_Command(
  uint8_t command,
  uint32_t address,
  uint8_t addressLength
);
static ErrorStatus w25qxx_WriteEnable(void);
static ErrorStatus w25qxx_ProgramPage(
  uint32_t address,
  const uint8_t* buffer,
  uint16_t length
);
static ErrorStatus w25qxx_EraseUnit(uint8_t command, uint32_t address);
static ErrorStatus w25qxx_WaitWhileBusy(void);




// -------------------------------------------------------------
ErrorStatus W25Qxx_Init(void) {
  uint8_t jedecId[3];

  w25qxxDevice.jedecId = 0U;
  w25qxxDevice.capacity = 0U;
  w25qxxDevice.blockCount = 0U;
  for (uint32_t index = 0U; index < sizeof(w25qxxDevice.uniqueId); index++) {
    w25qxxDevice.uniqueId[index] = 0U;
  }

  if (w25qxxDevice.spi != SPI2) return (ERROR);
  if (w25qxx_ChipSelectInit() != SUCCESS) return (ERROR);

  if (w25qxx_ReadCommand(
      W25QXX_COMMAND_READ_JEDEC_ID,
      0U,
      0U,
      0U,
      jedecId,
      sizeof(jedecId)
    ) != SUCCESS) {
    return (ERROR);
  }

  w25qxxDevice.jedecId = ((uint32_t)jedecId[0] << 16U)
    | ((uint32_t)jedecId[1] << 8U)
    | jedecId[2];

  if (w25qxxDevice.jedecId != W25Q64_JEDEC_ID) return (ERROR);

  if (w25qxx_ReadCommand(
      W25QXX_COMMAND_READ_UNIQUE_ID,
      0U,
      0U,
      W25QXX_UNIQUE_ID_DUMMY_LENGTH,
      w25qxxDevice.uniqueId,
      sizeof(w25qxxDevice.uniqueId)
    ) != SUCCESS) {
    return (ERROR);
  }

  w25qxxDevice.capacity = W25Q64_CAPACITY_BYTES;
  w25qxxDevice.blockCount =
    (uint16_t)(W25Q64_CAPACITY_BYTES / W25QXX_BLOCK_64K_SIZE);

  return (W25Qxx_SelfTest());
}




// -------------------------------------------------------------
ErrorStatus W25Qxx_SelfTest(void) {
  uint8_t buffer[W25QXX_PAGE_SIZE];

  if (W25Qxx_Erase(W25Q64_SELF_TEST_ADDRESS, 1U) != SUCCESS) {
    return (ERROR);
  }

  for (uint32_t index = 0U; index < sizeof(buffer); index++) {
    buffer[index] = (uint8_t)(
      W25QXX_SELF_TEST_SEED ^ (index * W25QXX_SELF_TEST_STEP)
    );
  }

  if (W25Qxx_Write(
      W25Q64_SELF_TEST_ADDRESS,
      buffer,
      sizeof(buffer)
    ) != SUCCESS) {
    return (ERROR);
  }

  for (uint32_t index = 0U; index < sizeof(buffer); index++) {
    buffer[index] = 0U;
  }

  if (W25Qxx_Read(
      W25Q64_SELF_TEST_ADDRESS,
      buffer,
      sizeof(buffer)
    ) != SUCCESS) {
    return (ERROR);
  }

  for (uint32_t index = 0U; index < sizeof(buffer); index++) {
    uint8_t expected = (uint8_t)(
      W25QXX_SELF_TEST_SEED ^ (index * W25QXX_SELF_TEST_STEP)
    );
    if (buffer[index] != expected) return (ERROR);
  }

  return (SUCCESS);
}




// -------------------------------------------------------------
W25Qxx_TypeDef* W25Qxx_GetDevice(void) {
  return (&w25qxxDevice);
}




// -------------------------------------------------------------
ErrorStatus W25Qxx_Read(
  uint32_t address,
  uint8_t* buffer,
  uint32_t length
) {
  uint32_t physicalAddress;

  if ((buffer == NULL) || (length == 0U)) return (ERROR);

  physicalAddress = w25qxx_DecodeAddress(address);
  if (w25qxx_ValidateRange(physicalAddress, length) != SUCCESS) return (ERROR);

  return (w25qxx_ReadCommand(
    W25QXX_COMMAND_FAST_READ,
    physicalAddress,
    W25QXX_ADDRESS_LENGTH,
    W25QXX_FAST_READ_DUMMY_LENGTH,
    buffer,
    length
  ));
}




// -------------------------------------------------------------
ErrorStatus W25Qxx_Write(
  uint32_t address,
  const uint8_t* buffer,
  uint32_t length
) {
  uint32_t physicalAddress;

  if ((buffer == NULL) || (length == 0U)) return (ERROR);

  physicalAddress = w25qxx_DecodeAddress(address);
  if (w25qxx_ValidateRange(physicalAddress, length) != SUCCESS) return (ERROR);

  while (length > 0U) {
    uint32_t pageRemaining =
      W25QXX_PAGE_SIZE - (physicalAddress % W25QXX_PAGE_SIZE);
    uint16_t chunk = (uint16_t)(
      (length < pageRemaining) ? length : pageRemaining
    );

    if (w25qxx_ProgramPage(physicalAddress, buffer, chunk) != SUCCESS) {
      return (ERROR);
    }
    if (w25qxx_WaitWhileBusy() != SUCCESS) return (ERROR);

    physicalAddress += chunk;
    buffer += chunk;
    length -= chunk;
  }

  return (SUCCESS);
}




// -------------------------------------------------------------
ErrorStatus W25Qxx_Erase(uint32_t address, uint32_t sectorCount) {
  uint32_t physicalAddress = w25qxx_DecodeAddress(address);
  uint32_t eraseLength;

  physicalAddress -= physicalAddress % W25QXX_SECTOR_SIZE;
  if ((sectorCount == 0U)
      || (sectorCount > (UINT32_MAX / W25QXX_SECTOR_SIZE))) {
    return (ERROR);
  }

  eraseLength = sectorCount * W25QXX_SECTOR_SIZE;
  if (w25qxx_ValidateRange(physicalAddress, eraseLength) != SUCCESS) {
    return (ERROR);
  }

  while (sectorCount > 0U) {
    uint8_t command = W25QXX_COMMAND_ERASE_SECTOR;
    uint32_t erasedSectors = 1U;

    if (((physicalAddress % W25QXX_BLOCK_64K_SIZE) == 0U)
        && (sectorCount >= 16U)) {
      command = W25QXX_COMMAND_ERASE_BLOCK_64K;
      erasedSectors = 16U;
    } else if (((physicalAddress % W25QXX_BLOCK_32K_SIZE) == 0U)
        && (sectorCount >= 8U)) {
      command = W25QXX_COMMAND_ERASE_BLOCK_32K;
      erasedSectors = 8U;
    }

    if (w25qxx_EraseUnit(command, physicalAddress) != SUCCESS) return (ERROR);

    physicalAddress += erasedSectors * W25QXX_SECTOR_SIZE;
    sectorCount -= erasedSectors;
  }

  return (SUCCESS);
}




// -------------------------------------------------------------
/**
  * @brief Configure the device-specific software chip-select GPIO.
  * @retval (ErrorStatus) SUCCESS when the configured port and pin are valid.
  *
  * The output latch is driven high before the pin becomes push-pull output so
  * that initialization cannot briefly select the flash.
  */
static ErrorStatus w25qxx_ChipSelectInit(void) {
  uint32_t shift;
  uint32_t mode = GPIO_GPO_PP | GPIO_IOS_50;

  if ((w25qxxDevice.chipSelectPort == NULL)
      || (w25qxxDevice.chipSelectPin > GPIO_PIN_15)) {
    return (ERROR);
  }

  W25QXX_RELEASE();

  if (w25qxxDevice.chipSelectPin < GPIO_PIN_8) {
    shift = w25qxxDevice.chipSelectPin * 4U;
    MODIFY_REG(
      w25qxxDevice.chipSelectPort->CRL,
      (0xfU << shift),
      (mode << shift)
    );
  } else {
    shift = (w25qxxDevice.chipSelectPin - GPIO_PIN_8) * 4U;
    MODIFY_REG(
      w25qxxDevice.chipSelectPort->CRH,
      (0xfU << shift),
      (mode << shift)
    );
  }

  return (SUCCESS);
}




// -------------------------------------------------------------
/**
  * @brief Enable SPI and select the flash for one complete command transaction.
  * @retval (ErrorStatus) SUCCESS when the SPI peripheral was enabled.
  */
static ErrorStatus w25qxx_BeginTransaction(void) {
  if (SPI_Enable(w25qxxDevice.spi) != SUCCESS) return (ERROR);
  W25QXX_SELECT();
  return (SUCCESS);
}




// -------------------------------------------------------------
/**
  * @brief Release the flash and disable SPI after a command transaction.
  * @param status (ErrorStatus) Status accumulated during the transaction.
  * @retval (ErrorStatus) ERROR when the transaction or SPI shutdown failed.
  */
static ErrorStatus w25qxx_EndTransaction(ErrorStatus status) {
  W25QXX_RELEASE();
  if (SPI_Disable(w25qxxDevice.spi) != SUCCESS) return (ERROR);
  return (status);
}




// -------------------------------------------------------------
/**
  * @brief Convert the legacy packed address into a physical byte address.
  * @param address (uint32_t) Address encoded as 0x00BBBBSP.
  * @retval (uint32_t) Physical byte offset within the flash.
  *
  * BBBB selects a 64 KB block, S selects one of its sixteen 4 KB sectors, and
  * P selects one of the sector's sixteen 256-byte pages.
  */
static uint32_t w25qxx_DecodeAddress(uint32_t address) {
  uint32_t block = (address >> 8U) & 0xffffU;
  uint32_t sector = (address >> 4U) & 0x0fU;
  uint32_t page = address & 0x0fU;

  return (block * W25QXX_BLOCK_64K_SIZE)
    + (sector * W25QXX_SECTOR_SIZE)
    + (page * W25QXX_PAGE_SIZE);
}




// -------------------------------------------------------------
/**
  * @brief Check that a physical byte range lies inside the detected flash.
  * @param address (uint32_t) Physical starting byte address.
  * @param length (uint32_t) Number of bytes in the requested range.
  * @retval (ErrorStatus) SUCCESS for a non-overflowing in-range request.
  */
static ErrorStatus w25qxx_ValidateRange(
  uint32_t address,
  uint32_t length
) {
  if ((w25qxxDevice.capacity == 0U)
      || (address >= w25qxxDevice.capacity)
      || (length > (w25qxxDevice.capacity - address))) {
    return (ERROR);
  }

  return (SUCCESS);
}




// -------------------------------------------------------------
/**
  * @brief Transfer one full-duplex SPI2 payload using DMA1 channels 4 and 5.
  * @param transmitBuffer (const uint8_t*) Source data, or NULL to send zeros.
  * @param receiveBuffer (uint8_t*) Destination, or NULL to discard received data.
  * @param length (uint16_t) Number of bytes to exchange.
  * @retval (ErrorStatus) Status of DMA completion and final SPI idle wait.
  *
  * Memory increment is disabled for the single dummy and discard bytes. This
  * avoids the out-of-bounds DMA access present in the original implementation.
  */
static ErrorStatus w25qxx_TransferDMA(
  const uint8_t* transmitBuffer,
  uint8_t* receiveBuffer,
  uint16_t length
) {
  uint8_t dummyTransmit = 0U;
  uint8_t discardReceive;
  uint32_t timeout = W25QXX_DMA_TIMEOUT;
  ErrorStatus status = SUCCESS;

  if (length == 0U) return (SUCCESS);

  CLEAR_BIT(DMA1_Channel4->CCR, DMA_CCR_EN);
  CLEAR_BIT(DMA1_Channel5->CCR, DMA_CCR_EN);
  DMA1->IFCR = DMA_IFCR_CGIF4 | DMA_IFCR_CGIF5;

  DMA1_Channel4->CPAR = (uint32_t)&w25qxxDevice.spi->DR;
  DMA1_Channel4->CMAR = (uint32_t)(
    (receiveBuffer != NULL) ? receiveBuffer : &discardReceive
  );
  DMA1_Channel4->CNDTR = length;
  DMA1_Channel4->CCR = DMA_CCR_PL_1
    | ((receiveBuffer != NULL) ? DMA_CCR_MINC : 0U);

  DMA1_Channel5->CPAR = (uint32_t)&w25qxxDevice.spi->DR;
  DMA1_Channel5->CMAR = (uint32_t)(
    (transmitBuffer != NULL) ? transmitBuffer : &dummyTransmit
  );
  DMA1_Channel5->CNDTR = length;
  DMA1_Channel5->CCR = DMA_CCR_DIR
    | DMA_CCR_PL_1
    | ((transmitBuffer != NULL) ? DMA_CCR_MINC : 0U);

  SET_BIT(
    w25qxxDevice.spi->CR2,
    SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN
  );
  SET_BIT(DMA1_Channel4->CCR, DMA_CCR_EN);
  SET_BIT(DMA1_Channel5->CCR, DMA_CCR_EN);

  while (((DMA1->ISR & DMA_ISR_TCIF4) == 0U)
      || ((DMA1->ISR & DMA_ISR_TCIF5) == 0U)) {
    if ((DMA1->ISR & (DMA_ISR_TEIF4 | DMA_ISR_TEIF5)) != 0U) {
      status = ERROR;
      break;
    }
    if (--timeout == 0U) {
      status = ERROR;
      break;
    }
  }

  while ((status == SUCCESS)
      && PREG_CHECK(w25qxxDevice.spi->SR, SPI_SR_BSY_Pos)) {
    if (--timeout == 0U) status = ERROR;
  }

  CLEAR_BIT(DMA1_Channel4->CCR, DMA_CCR_EN);
  CLEAR_BIT(DMA1_Channel5->CCR, DMA_CCR_EN);
  CLEAR_BIT(
    w25qxxDevice.spi->CR2,
    SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN
  );
  DMA1->IFCR = DMA_IFCR_CGIF4 | DMA_IFCR_CGIF5;

  return (status);
}




// -------------------------------------------------------------
/**
  * @brief Send a command byte followed by optional address and dummy bytes.
  * @param command (uint8_t) Winbond command opcode.
  * @param address (uint32_t) Physical 24-bit flash address.
  * @param addressLength (uint8_t) Number of address bytes to send.
  * @param dummyLength (uint8_t) Number of zero-valued dummy bytes to send.
  * @retval (ErrorStatus) Status of the polling SPI transfers.
  *
  * The caller must already own an active transaction with chip select low.
  */
static ErrorStatus w25qxx_SendHeader(
  uint8_t command,
  uint32_t address,
  uint8_t addressLength,
  uint8_t dummyLength
) {
  uint8_t addressBytes[3];
  uint8_t dummy = 0U;

  if (addressLength > sizeof(addressBytes)) return (ERROR);
  if (SPI_Write8(w25qxxDevice.spi, &command, 1U) != SUCCESS) return (ERROR);

  if (addressLength > 0U) {
    addressBytes[0] = (uint8_t)(address >> 16U);
    addressBytes[1] = (uint8_t)(address >> 8U);
    addressBytes[2] = (uint8_t)address;
    if (SPI_Write8(
        w25qxxDevice.spi,
        addressBytes,
        addressLength
      ) != SUCCESS) {
      return (ERROR);
    }
  }

  while (dummyLength-- > 0U) {
    if (SPI_Write8(w25qxxDevice.spi, &dummy, 1U) != SUCCESS) return (ERROR);
  }

  return (SUCCESS);
}




// -------------------------------------------------------------
/**
  * @brief Execute a command that returns a payload through DMA.
  * @param command (uint8_t) Winbond read command opcode.
  * @param address (uint32_t) Physical address when required by the command.
  * @param addressLength (uint8_t) Number of address bytes to send.
  * @param dummyLength (uint8_t) Number of command-specific dummy bytes.
  * @param buffer (uint8_t*) Destination buffer.
  * @param length (uint32_t) Number of payload bytes to receive.
  * @retval (ErrorStatus) Status of the complete transaction.
  */
static ErrorStatus w25qxx_ReadCommand(
  uint8_t command,
  uint32_t address,
  uint8_t addressLength,
  uint8_t dummyLength,
  uint8_t* buffer,
  uint32_t length
) {
  ErrorStatus status;

  if ((buffer == NULL) || (length == 0U)) return (ERROR);
  if (w25qxx_BeginTransaction() != SUCCESS) return (ERROR);

  status = w25qxx_SendHeader(
    command,
    address,
    addressLength,
    dummyLength
  );

  while ((status == SUCCESS) && (length > 0U)) {
    uint16_t chunk = (uint16_t)(
      (length > W25QXX_DMA_TRANSFER_LIMIT)
        ? W25QXX_DMA_TRANSFER_LIMIT
        : length
    );
    status = w25qxx_TransferDMA(NULL, buffer, chunk);
    buffer += chunk;
    length -= chunk;
  }

  return (w25qxx_EndTransaction(status));
}




// -------------------------------------------------------------
/**
  * @brief Execute a command that has no returned payload.
  * @param command (uint8_t) Winbond command opcode.
  * @param address (uint32_t) Physical address when required.
  * @param addressLength (uint8_t) Number of address bytes to send.
  * @retval (ErrorStatus) Status of the complete transaction.
  */
static ErrorStatus w25qxx_Command(
  uint8_t command,
  uint32_t address,
  uint8_t addressLength
) {
  ErrorStatus status;

  if (w25qxx_BeginTransaction() != SUCCESS) return (ERROR);

  status = w25qxx_SendHeader(command, address, addressLength, 0U);
  return (w25qxx_EndTransaction(status));
}




// -------------------------------------------------------------
/**
  * @brief Set the flash write-enable latch before program or erase.
  * @retval (ErrorStatus) Status of the write-enable command transaction.
  */
static ErrorStatus w25qxx_WriteEnable(void) {
  return (w25qxx_Command(
    W25QXX_COMMAND_WRITE_ENABLE,
    0U,
    0U
  ));
}




// -------------------------------------------------------------
/**
  * @brief Program data without crossing the current 256-byte page boundary.
  * @param address (uint32_t) Physical starting byte address.
  * @param buffer (const uint8_t*) Source bytes.
  * @param length (uint16_t) Bytes to program within the current page.
  * @retval (ErrorStatus) Status of write-enable, header, and DMA transfer.
  */
static ErrorStatus w25qxx_ProgramPage(
  uint32_t address,
  const uint8_t* buffer,
  uint16_t length
) {
  ErrorStatus status;

  if ((buffer == NULL)
      || (length == 0U)
      || (length > W25QXX_PAGE_SIZE)
      || ((address % W25QXX_PAGE_SIZE) + length > W25QXX_PAGE_SIZE)) {
    return (ERROR);
  }

  if (w25qxx_WriteEnable() != SUCCESS) return (ERROR);
  if (w25qxx_BeginTransaction() != SUCCESS) return (ERROR);

  status = w25qxx_SendHeader(
    W25QXX_COMMAND_PAGE_PROGRAM,
    address,
    W25QXX_ADDRESS_LENGTH,
    0U
  );
  if (status == SUCCESS) {
    status = w25qxx_TransferDMA(buffer, NULL, length);
  }

  return (w25qxx_EndTransaction(status));
}




// -------------------------------------------------------------
/**
  * @brief Erase one aligned sector or block and wait for completion.
  * @param command (uint8_t) Sector, 32 KB block, or 64 KB block opcode.
  * @param address (uint32_t) Physical address aligned for the selected opcode.
  * @retval (ErrorStatus) Status of write-enable, erase, and busy polling.
  */
static ErrorStatus w25qxx_EraseUnit(uint8_t command, uint32_t address) {
  if ((command != W25QXX_COMMAND_ERASE_SECTOR)
      && (command != W25QXX_COMMAND_ERASE_BLOCK_32K)
      && (command != W25QXX_COMMAND_ERASE_BLOCK_64K)) {
    return (ERROR);
  }

  if (w25qxx_WriteEnable() != SUCCESS) return (ERROR);
  if (w25qxx_Command(
      command,
      address,
      W25QXX_ADDRESS_LENGTH
    ) != SUCCESS) {
    return (ERROR);
  }

  return (w25qxx_WaitWhileBusy());
}




// -------------------------------------------------------------
/**
  * @brief Poll status register 1 until program or erase completes.
  * @retval (ErrorStatus) SUCCESS when BUSY clears before the polling limit.
  */
static ErrorStatus w25qxx_WaitWhileBusy(void) {
  uint8_t statusRegister = W25QXX_STATUS_BUSY;
  uint32_t remainingPolls = W25QXX_BUSY_POLL_LIMIT;

  while ((statusRegister & W25QXX_STATUS_BUSY) != 0U) {
    if (w25qxx_ReadCommand(
        W25QXX_COMMAND_READ_STATUS_1,
        0U,
        0U,
        0U,
        &statusRegister,
        1U
      ) != SUCCESS) {
      return (ERROR);
    }
    if (--remainingPolls == 0U) return (ERROR);
  }

  return (SUCCESS);
}
