/**
  ******************************************************************************
  * @file           : w25qxx.h
  * @brief          : Winbond W25Qxx SPI NOR flash interface.
  * @project        : STM32F1 Health Check Device
  * @platform       : STMicroelectronics STM32F103C8
  * @created        : 27.07.2026 09:52:45 AM
  ******************************************************************************
  * @attention
  * @copyright  : 2017-2026, Dmitry Slobodchikov
  ******************************************************************************
  */

#ifndef __W25QXX_H
#define __W25QXX_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "misc.h"
#include "stm32f103xb.h"

#define W25QXX_PAGE_SIZE              256U
#define W25QXX_SECTOR_SIZE           4096U
#define W25QXX_BLOCK_32K_SIZE       32768U
#define W25QXX_BLOCK_64K_SIZE       65536U

#define W25QXX_PACK_ADDRESS(block, sector, page) ( \
  (((uint32_t)(block) & 0xffffU) << 8U) \
  | (((uint32_t)(sector) & 0x0fU) << 4U) \
  | ((uint32_t)(page) & 0x0fU) \
)

#define W25Q64_JEDEC_ID          0xEF4017UL
#define W25Q64_CAPACITY_BYTES    0x00800000UL

/*
 * Packed address of the final 4 KB sector, reserved for the startup self-test.
 * The address follows the 0x00BBBBSP block/sector/page convention.
 */
#define W25Q64_SELF_TEST_ADDRESS       0x00007ff0UL

#define W25Q64_DEFAULT_CS_PORT         GPIOB
#define W25Q64_DEFAULT_CS_PIN          GPIO_PIN_12

/**
  * @brief Runtime identity and geometry of a detected W25Qxx flash.
  * @param jedecId (uint32_t) 24-bit manufacturer, memory-type, and capacity ID.
  * @param uniqueId (uint8_t[8]) Factory-programmed 64-bit unique identifier.
  * @param capacity (uint32_t) Flash capacity in bytes.
  * @param blockCount (uint16_t) Number of 64 KB blocks.
  * @param spi (SPI_TypeDef*) SPI peripheral; the current DMA mapping requires SPI2.
  * @param chipSelectPort (GPIO_TypeDef*) GPIO port controlling flash /CS.
  * @param chipSelectPin (uint16_t) GPIO pin number controlling flash /CS.
  */
typedef struct {
  uint32_t jedecId;
  uint8_t uniqueId[8];
  uint32_t capacity;
  uint16_t blockCount;
  SPI_TypeDef* spi;
  GPIO_TypeDef* chipSelectPort;
  uint16_t chipSelectPin;
} W25Qxx_TypeDef;

/**
  * @brief Initialize and identify the W25Q64FV connected to SPI2.
  * @retval (ErrorStatus) SUCCESS when the expected JEDEC ID is detected.
  *
  * Initialization configures the software-controlled chip-select pin, reads
  * the JEDEC and unique identifiers, establishes the device geometry, and
  * performs the destructive startup self-test in the reserved final sector.
  */
ErrorStatus W25Qxx_Init(void);

/**
  * @brief Verify erase, page-program, and read operations in the test sector.
  * @retval (ErrorStatus) SUCCESS when the programmed pattern is read correctly.
  *
  * This operation erases the final 4 KB flash sector and leaves the test
  * pattern in its first page. The sector is reserved exclusively for testing.
  */
ErrorStatus W25Qxx_SelfTest(void);

/**
  * @brief Return the statically allocated flash device state.
  * @retval (W25Qxx_TypeDef*) Detected device state.
  */
W25Qxx_TypeDef* W25Qxx_GetDevice(void);

/**
  * @brief Read bytes from a packed block/sector/page address using SPI2 DMA.
  * @param address (uint32_t) Packed address in the 0x00BBBBSP format.
  * @param buffer (uint8_t*) Destination buffer owned by the caller.
  * @param length (uint32_t) Number of bytes to read.
  * @retval (ErrorStatus) Status of the operation.
  *
  * The packed address identifies the starting 256-byte page. Reads may
  * continue across page and sector boundaries up to the detected capacity.
  */
ErrorStatus W25Qxx_Read(uint32_t address, uint8_t* buffer, uint32_t length);

/**
  * @brief Program bytes from a packed block/sector/page address.
  * @param address (uint32_t) Packed address in the 0x00BBBBSP format.
  * @param buffer (const uint8_t*) Source buffer owned by the caller.
  * @param length (uint32_t) Number of bytes to program.
  * @retval (ErrorStatus) Status of the operation.
  *
  * The affected sectors must be erased before programming. The driver splits
  * the transfer at every page boundary because a page-program command cannot
  * cross a 256-byte page.
  */
ErrorStatus W25Qxx_Write(
  uint32_t address,
  const uint8_t* buffer,
  uint32_t length
);

/**
  * @brief Erase sectors beginning at a packed block/sector/page address.
  * @param address (uint32_t) Packed address; page bits are ignored.
  * @param sectorCount (uint32_t) Number of consecutive 4 KB sectors to erase.
  * @retval (ErrorStatus) Status of the operation.
  *
  * The implementation selects aligned 32 KB and 64 KB erase commands where
  * possible and falls back to individual 4 KB sectors elsewhere.
  */
ErrorStatus W25Qxx_Erase(uint32_t address, uint32_t sectorCount);

#ifdef __cplusplus
}
#endif

#endif /* __W25QXX_H */
