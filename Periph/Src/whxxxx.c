/**
  ******************************************************************************
  * @file           : whxxxx.c
  * @brief          : WHxxxx character display interface over an I2C backpack.
  ******************************************************************************
  */

#include "whxxxx.h"
#include "i2c.h"

#define WHXXXX_BACKLIGHT       (1U << 3U)
#define WHXXXX_ENABLE          (1U << 2U)
#define WHXXXX_REGISTER_SELECT (1U << 0U)

#if (WH_DSPL_MODEL == 1602)
#define WHXXXX_COLUMNS 16U
#define WHXXXX_ROWS    2U
#elif (WH_DSPL_MODEL == 2004)
#define WHXXXX_COLUMNS 20U
#define WHXXXX_ROWS    4U
#else
#error "Unsupported WHxxxx display model"
#endif

static I2C_TypeDef* displayI2C;
static uint8_t displayAddress;
static uint8_t displayRow;
static uint8_t displayColumn;
static SemaphoreHandle_t displayMutex;
static StaticSemaphore_t displayMutexBuffer;

static ErrorStatus whxxxx_WriteNibble(uint8_t, uint8_t);
static ErrorStatus whxxxx_WriteByte(uint8_t, uint8_t);
static ErrorStatus whxxxx_Command(uint8_t);
static ErrorStatus whxxxx_SetCursor(uint8_t, uint8_t);
static ErrorStatus whxxxx_Lock(void);
static void whxxxx_Unlock(void);




// -------------------------------------------------------------
ErrorStatus WHxxxx_Init(I2C_TypeDef* i2c, uint8_t address) {
  displayI2C = i2c;
  displayAddress = address;
  displayRow = 0U;
  displayColumn = 0U;
  displayMutex = xSemaphoreCreateMutexStatic(&displayMutexBuffer);
  if (displayMutex == NULL) return (ERROR);

  _delay_ms(40U);
  if (whxxxx_WriteNibble(0x30U, 0U) != SUCCESS) return (ERROR);
  _delay_us(4100U);
  if (whxxxx_WriteNibble(0x30U, 0U) != SUCCESS) return (ERROR);
  _delay_us(100U);
  if (whxxxx_WriteNibble(0x30U, 0U) != SUCCESS) return (ERROR);
  _delay_us(40U);
  if (whxxxx_WriteNibble(0x20U, 0U) != SUCCESS) return (ERROR);
  _delay_us(40U);

  if (whxxxx_Command(0x20U) != SUCCESS) return (ERROR); /* 4-bit, 1-line font mode */
  if (whxxxx_Command(0x0CU) != SUCCESS) return (ERROR); /* Display on, cursor off */
  if (whxxxx_Command(0x01U) != SUCCESS) return (ERROR); /* Clear display */
  _delay_us(1640U);
  if (whxxxx_Command(0x06U) != SUCCESS) return (ERROR); /* Increment cursor */

  return (SUCCESS);
}




// -------------------------------------------------------------
ErrorStatus WHxxxx_Clear(void) {
  if (whxxxx_Lock() != SUCCESS) return (ERROR);
  ErrorStatus status = whxxxx_Command(0x01U);
  if (status == SUCCESS) {
    _delay_us(1640U);
    displayRow = 0U;
    displayColumn = 0U;
  }
  whxxxx_Unlock();
  return (status);
}




// -------------------------------------------------------------
ErrorStatus WHxxxx_Print(const uint8_t* buffer, uint16_t length) {
  if (buffer == NULL) return (ERROR);
  if (whxxxx_Lock() != SUCCESS) return (ERROR);

  ErrorStatus status = SUCCESS;
  for (uint16_t i = 0U; (i < length) && (status == SUCCESS); i++) {
    if (whxxxx_WriteByte(buffer[i], WHXXXX_REGISTER_SELECT) != SUCCESS) {
      status = ERROR;
    }
  }

  whxxxx_Unlock();
  return (status);
}




// -------------------------------------------------------------
int putc_dspl_wh(char character) {
  if ((displayI2C == NULL) || (displayMutex == NULL)) return (ERROR);
  if (whxxxx_Lock() != SUCCESS) return (ERROR);

  ErrorStatus status = SUCCESS;
  if (character == '\n') {
    displayRow++;
    displayColumn = 0U;
    if (displayRow >= WHXXXX_ROWS) {
      status = whxxxx_Command(0x01U);
      _delay_us(1640U);
      displayRow = 0U;
    } else {
      status = whxxxx_SetCursor(displayRow, displayColumn);
    }
  } else if (character != '\r') {
    if (displayColumn >= WHXXXX_COLUMNS) {
      displayRow++;
      displayColumn = 0U;
      if (displayRow >= WHXXXX_ROWS) {
        status = whxxxx_Command(0x01U);
        _delay_us(1640U);
        displayRow = 0U;
      } else {
        status = whxxxx_SetCursor(displayRow, displayColumn);
      }
    }
    if (status == SUCCESS) {
      status = whxxxx_WriteByte((uint8_t)character, WHXXXX_REGISTER_SELECT);
      displayColumn++;
    }
  }

  whxxxx_Unlock();
  return (status == SUCCESS) ? (uint8_t)character : ERROR;
}




// -------------------------------------------------------------
static ErrorStatus whxxxx_WriteNibble(uint8_t value, uint8_t flags) {
  uint8_t bytes[2];
  uint8_t output = (value & 0xF0U) | WHXXXX_BACKLIGHT | flags;
  bytes[0] = output | WHXXXX_ENABLE;
  bytes[1] = output;
  return I2C_Master_Send(displayI2C, displayAddress, bytes, sizeof(bytes));
}




// -------------------------------------------------------------
static ErrorStatus whxxxx_WriteByte(uint8_t value, uint8_t flags) {
  uint8_t bytes[4];
  uint8_t high = (value & 0xF0U) | WHXXXX_BACKLIGHT | flags;
  uint8_t low = ((value << 4U) & 0xF0U) | WHXXXX_BACKLIGHT | flags;
  bytes[0] = high | WHXXXX_ENABLE;
  bytes[1] = high;
  bytes[2] = low | WHXXXX_ENABLE;
  bytes[3] = low;
  ErrorStatus status = I2C_Master_Send(displayI2C, displayAddress, bytes, sizeof(bytes));
  _delay_us(40U);
  return (status);
}




// -------------------------------------------------------------
static ErrorStatus whxxxx_Command(uint8_t command) {
  return whxxxx_WriteByte(command, 0U);
}




// -------------------------------------------------------------
static ErrorStatus whxxxx_SetCursor(uint8_t row, uint8_t column) {
#if (WH_DSPL_MODEL == 1602)
  static const uint8_t rowOffsets[WHXXXX_ROWS] = {0x00U, 0x40U};
#else
  static const uint8_t rowOffsets[WHXXXX_ROWS] = {0x00U, 0x40U, 0x14U, 0x54U};
#endif
  return whxxxx_Command(0x80U | (rowOffsets[row] + column));
}




// -------------------------------------------------------------
static ErrorStatus whxxxx_Lock(void) {
  return (xSemaphoreTake(displayMutex, portMAX_DELAY) == pdTRUE) ? SUCCESS : ERROR;
}




// -------------------------------------------------------------
static void whxxxx_Unlock(void) {
  (void)xSemaphoreGive(displayMutex);
}
