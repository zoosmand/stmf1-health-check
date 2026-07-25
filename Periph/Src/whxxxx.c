/**
  ******************************************************************************
  * @file           : whxxxx.c
  * @brief          : WHxxxx character display interface over an I2C backpack.
  ******************************************************************************
  */

#include "whxxxx.h"
#include "i2c.h"

#if defined(USE_WH_DISPLAY)

#define WHXXXX_BACKLIGHT       (1U << 3U)
#define WHXXXX_ENABLE          (1U << 2U)
#define WHXXXX_REGISTER_SELECT (1U << 0U)
#define WHXXXX_DISPLAY_SHIFT_LEFT 0x18U

#if (WH_DSPL_MODEL == 1602)
#define WHXXXX_COLUMNS           16U
#define WHXXXX_ROWS              2U
#define WHXXXX_ONE_LINE_LENGTH   24U
#define WHXXXX_ONE_LINE_SHIFT    1U
#elif (WH_DSPL_MODEL == 2004)
#define WHXXXX_COLUMNS           20U
#define WHXXXX_ROWS              4U
#define WHXXXX_ONE_LINE_LENGTH   40U
#define WHXXXX_ONE_LINE_SHIFT    0U
#else
#error "Unsupported WHxxxx display model"
#endif

#if (WH_DSPL_LINE_MODE == 1)
#define WHXXXX_FUNCTION_SET 0x20U
#elif (WH_DSPL_LINE_MODE == 2)
#define WHXXXX_FUNCTION_SET 0x28U
#else
#error "WH_DSPL_LINE_MODE must be 1 or 2"
#endif

static I2C_TypeDef* displayI2C;
static uint8_t displayAddress;
static uint8_t displayRow;
static uint8_t displayColumn;
static FunctionalState displayReady;
static FunctionalState displayNewLinePending;
#if (WH_DSPL_LINE_MODE == 2)
static uint8_t displayLines[WHXXXX_ROWS][WHXXXX_COLUMNS];
#endif
static SemaphoreHandle_t displayMutex;
static StaticSemaphore_t displayMutexBuffer;

static ErrorStatus whxxxx_WriteNibble(uint8_t, uint8_t);
static ErrorStatus whxxxx_WriteByte(uint8_t, uint8_t);
static ErrorStatus whxxxx_Command(uint8_t);
#if (WH_DSPL_LINE_MODE == 2)
static ErrorStatus whxxxx_SetCursor(uint8_t, uint8_t);
static ErrorStatus whxxxx_StartPrintfLine(void);
#endif
static ErrorStatus whxxxx_Lock(void);
static void whxxxx_Unlock(void);




// -------------------------------------------------------------
ErrorStatus WHxxxx_Init(I2C_TypeDef* i2c, uint8_t address) {
  displayReady = DISABLE;
  displayI2C = i2c;
  displayAddress = address;
  displayRow = 0U;
  displayColumn = 0U;
  displayNewLinePending = DISABLE;
#if (WH_DSPL_LINE_MODE == 2)
  for (uint8_t row = 0U; row < WHXXXX_ROWS; row++) {
    for (uint8_t column = 0U; column < WHXXXX_COLUMNS; column++) {
      displayLines[row][column] = ' ';
    }
  }
  displayNewLinePending = ENABLE;
#endif
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

  if (whxxxx_Command(WHXXXX_FUNCTION_SET) != SUCCESS) return (ERROR);
  if (whxxxx_Command(0x0CU) != SUCCESS) return (ERROR); /* Display on, cursor off */
  if (whxxxx_Command(0x01U) != SUCCESS) return (ERROR); /* Clear display */
  _delay_us(1640U);
  if (whxxxx_Command(0x06U) != SUCCESS) return (ERROR); /* Increment cursor */

  displayReady = ENABLE;
  return (SUCCESS);
}




// -------------------------------------------------------------
ErrorStatus WHxxxx_Clear(void) {
  if (displayReady != ENABLE) return (ERROR);
  if (whxxxx_Lock() != SUCCESS) return (ERROR);
  ErrorStatus status = whxxxx_Command(0x01U);
  if (status == SUCCESS) {
    _delay_us(1640U);
    displayRow = 0U;
    displayColumn = 0U;
    displayNewLinePending = DISABLE;
#if (WH_DSPL_LINE_MODE == 2)
    for (uint8_t row = 0U; row < WHXXXX_ROWS; row++) {
      for (uint8_t column = 0U; column < WHXXXX_COLUMNS; column++) {
        displayLines[row][column] = ' ';
      }
    }
    displayNewLinePending = ENABLE;
#endif
  }
  whxxxx_Unlock();
  return (status);
}




// -------------------------------------------------------------
ErrorStatus WHxxxx_Print(const uint8_t* buffer, uint16_t length) {
  if ((displayReady != ENABLE) || (buffer == NULL)) return (ERROR);
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
  if ((displayReady != ENABLE) || (displayI2C == NULL) || (displayMutex == NULL)) {
    return (ERROR);
  }
  if (whxxxx_Lock() != SUCCESS) return (ERROR);

  ErrorStatus status = SUCCESS;
#if (WH_DSPL_LINE_MODE == 1)
  if (character == '\n') {
    displayNewLinePending = ENABLE;
  } else if (character != '\r') {
    if (displayNewLinePending == ENABLE) {
      status = whxxxx_Command(0x01U);
      _delay_us(1640U);
      displayRow = 0U;
      displayColumn = 0U;
      displayNewLinePending = DISABLE;
    }
    if ((status == SUCCESS)
        && (displayColumn < WHXXXX_ONE_LINE_LENGTH)) {
      status = whxxxx_WriteByte((uint8_t)character, WHXXXX_REGISTER_SELECT);
      if (status == SUCCESS) {
        displayColumn++;
#if (WHXXXX_ONE_LINE_SHIFT == 1U)
        if (displayColumn > WHXXXX_COLUMNS) {
          status = whxxxx_Command(WHXXXX_DISPLAY_SHIFT_LEFT);
        }
#endif
      }
    }
  }
#else
  if (character == '\n') {
    displayNewLinePending = ENABLE;
  } else if (character != '\r') {
    if (displayNewLinePending == ENABLE) {
      status = whxxxx_StartPrintfLine();
    }
    if ((status == SUCCESS)
        && (displayColumn < WHXXXX_COLUMNS)) {
      status = whxxxx_WriteByte((uint8_t)character, WHXXXX_REGISTER_SELECT);
      if (status == SUCCESS) {
        displayLines[WHXXXX_ROWS - 1U][displayColumn] = (uint8_t)character;
        displayColumn++;
      }
    }
  }
#endif

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
#if (WH_DSPL_LINE_MODE == 2)
static ErrorStatus whxxxx_SetCursor(uint8_t row, uint8_t column) {
#if (WH_DSPL_MODEL == 1602)
  static const uint8_t rowOffsets[WHXXXX_ROWS] = {0x00U, 0x40U};
#else
  static const uint8_t rowOffsets[WHXXXX_ROWS] = {0x00U, 0x40U, 0x14U, 0x54U};
#endif
  return whxxxx_Command(0x80U | (rowOffsets[row] + column));
}




// -------------------------------------------------------------
static ErrorStatus whxxxx_StartPrintfLine(void) {
  for (uint8_t row = 0U; row < (WHXXXX_ROWS - 1U); row++) {
    for (uint8_t column = 0U; column < WHXXXX_COLUMNS; column++) {
      displayLines[row][column] = displayLines[row + 1U][column];
    }
  }
  for (uint8_t column = 0U; column < WHXXXX_COLUMNS; column++) {
    displayLines[WHXXXX_ROWS - 1U][column] = ' ';
  }

  ErrorStatus status = SUCCESS;
  for (uint8_t row = 0U;
       (row < WHXXXX_ROWS) && (status == SUCCESS);
       row++) {
    status = whxxxx_SetCursor(row, 0U);
    for (uint8_t column = 0U;
         (column < WHXXXX_COLUMNS) && (status == SUCCESS);
         column++) {
      status = whxxxx_WriteByte(
        displayLines[row][column],
        WHXXXX_REGISTER_SELECT
      );
    }
  }

  if (status == SUCCESS) {
    status = whxxxx_SetCursor(WHXXXX_ROWS - 1U, 0U);
  }
  if (status == SUCCESS) {
    displayRow = WHXXXX_ROWS - 1U;
    displayColumn = 0U;
    displayNewLinePending = DISABLE;
  }
  return (status);
}
#endif




// -------------------------------------------------------------
static ErrorStatus whxxxx_Lock(void) {
  return (xSemaphoreTake(displayMutex, portMAX_DELAY) == pdTRUE) ? SUCCESS : ERROR;
}




// -------------------------------------------------------------
static void whxxxx_Unlock(void) {
  (void)xSemaphoreGive(displayMutex);
}

#endif /* USE_WH_DISPLAY */
