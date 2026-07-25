/**
  ******************************************************************************
  * @file           : ssd13xx.c
  * @brief          : SSD1306/SSD1315 display interface over I2C.
  ******************************************************************************
  */

#include "ssd13xx.h"
#include "fonts.h"
#include "i2c.h"

#if defined(USE_SSD_DISPLAY)

#define SSD13XX_COMMAND_CONTROL 0x00U
#define SSD13XX_DATA_CONTROL    0x40U
#define SSD13XX_PAGE_COUNT      (SSD13XX_HEIGHT / 8U)
#define SSD13XX_MAX_GLYPH_BYTES 24U

static SSD13xx_TypeDef* displayDevice;
static uint8_t displayColumn;
static uint8_t displayRow;
static SemaphoreHandle_t displayMutex;
static StaticSemaphore_t displayMutexBuffer;

static ErrorStatus ssd13xx_SendCommands(
  SSD13xx_TypeDef*,
  const uint8_t*,
  uint8_t
);
static ErrorStatus ssd13xx_SetWindow(
  SSD13xx_TypeDef*,
  uint8_t,
  uint8_t,
  uint8_t,
  uint8_t,
  uint8_t
);
static ErrorStatus ssd13xx_ClearTextRow(SSD13xx_TypeDef*, uint8_t);
static ErrorStatus ssd13xx_WriteCharacter(SSD13xx_TypeDef*, uint8_t);
static uint8_t ssd13xx_FontIndex(uint8_t);




// -------------------------------------------------------------
ErrorStatus SSD13xx_Init(
  SSD13xx_TypeDef* device,
  I2C_TypeDef* i2c,
  uint8_t address,
  uint16_t model,
  SSD13xx_Font_TypeDef font
) {
  if ((device == NULL)
      || (i2c != I2C1)
      || ((model != SSD1306_MODEL) && (model != SSD1315_MODEL))
      || ((font != SSD13XX_FONT_5X7) && (font != SSD13XX_FONT_10X14))) {
    return (ERROR);
  }

  displayMutex = xSemaphoreCreateMutexStatic(&displayMutexBuffer);
  if (displayMutex == NULL) return (ERROR);

  device->I2C = i2c;
  device->Address = address;
  device->Model = model;
  device->Font = font;
  displayDevice = device;
  displayColumn = 0U;
  displayRow = 0U;

  static const uint8_t initCommands[] = {
    0xAEU, 0xD5U, 0x80U, 0xA8U, 0x3FU, 0xD3U, 0x00U, 0x40U,
    0x8DU, 0x14U, 0xA1U, 0xC0U, 0xDAU, 0x12U, 0x81U, 0x7FU,
    0xD9U, 0x88U, 0xDBU, 0x20U, 0xA4U, 0xA6U, 0xAFU
  };

  _delay_ms(15U);
  if (ssd13xx_SendCommands(
        device,
        initCommands,
        sizeof(initCommands)
      ) != SUCCESS) {
    displayDevice = NULL;
    return (ERROR);
  }
  if (SSD13xx_Clear(device) != SUCCESS) {
    displayDevice = NULL;
    return (ERROR);
  }
  return (SUCCESS);
}




// -------------------------------------------------------------
ErrorStatus SSD13xx_Clear(SSD13xx_TypeDef* device) {
  if ((device == NULL) || (displayMutex == NULL)) return (ERROR);
  if (xSemaphoreTake(displayMutex, portMAX_DELAY) != pdTRUE) return (ERROR);

  ErrorStatus status = ssd13xx_SetWindow(
    device,
    0x00U,
    0U,
    SSD13XX_WIDTH - 1U,
    0U,
    SSD13XX_PAGE_COUNT - 1U
  );
  uint8_t clearBuffer[17] = {SSD13XX_DATA_CONTROL};
  for (uint8_t block = 0U; (block < 64U) && (status == SUCCESS); block++) {
    status = I2C_Master_Send(
      device->I2C,
      device->Address,
      clearBuffer,
      sizeof(clearBuffer)
    );
  }
  displayColumn = 0U;
  displayRow = 0U;
  (void)xSemaphoreGive(displayMutex);
  return (status);
}




// -------------------------------------------------------------
int putc_dspl_ssd(char character) {
  if ((displayDevice == NULL) || (displayMutex == NULL)) return (ERROR);
  if (xSemaphoreTake(displayMutex, portMAX_DELAY) != pdTRUE) return (ERROR);

  ErrorStatus status = SUCCESS;
  uint8_t columns = (displayDevice->Font == SSD13XX_FONT_10X14) ? 10U : 21U;
  uint8_t rows = (displayDevice->Font == SSD13XX_FONT_10X14) ? 4U : 8U;

  if (character == '\n') {
    displayColumn = 0U;
    displayRow = (displayRow + 1U) % rows;
    status = ssd13xx_ClearTextRow(displayDevice, displayRow);
  } else if (character != '\r') {
    if (displayColumn < columns) {
      status = ssd13xx_WriteCharacter(displayDevice, (uint8_t)character);
      if (status == SUCCESS) displayColumn++;
    }
  }

  (void)xSemaphoreGive(displayMutex);
  return (status == SUCCESS) ? (uint8_t)character : ERROR;
}




// -------------------------------------------------------------
static ErrorStatus ssd13xx_SendCommands(
  SSD13xx_TypeDef* device,
  const uint8_t* commands,
  uint8_t length
) {
  if ((commands == NULL) || (length == 0U) || (length > 31U)) return (ERROR);
  uint8_t buffer[32];
  buffer[0] = SSD13XX_COMMAND_CONTROL;
  for (uint8_t i = 0U; i < length; i++) buffer[i + 1U] = commands[i];
  return I2C_Master_Send(device->I2C, device->Address, buffer, length + 1U);
}




// -------------------------------------------------------------
static ErrorStatus ssd13xx_SetWindow(
  SSD13xx_TypeDef* device,
  uint8_t addressingMode,
  uint8_t firstColumn,
  uint8_t lastColumn,
  uint8_t firstPage,
  uint8_t lastPage
) {
  const uint8_t commands[] = {
    0x20U, addressingMode,
    0x21U, firstColumn, lastColumn,
    0x22U, firstPage, lastPage
  };
  return ssd13xx_SendCommands(device, commands, sizeof(commands));
}




// -------------------------------------------------------------
static ErrorStatus ssd13xx_ClearTextRow(
  SSD13xx_TypeDef* device,
  uint8_t row
) {
  uint8_t pageCount = (device->Font == SSD13XX_FONT_10X14) ? 2U : 1U;
  uint8_t firstPage = row * pageCount;
  ErrorStatus status = ssd13xx_SetWindow(
    device,
    0x00U,
    0U,
    SSD13XX_WIDTH - 1U,
    firstPage,
    firstPage + pageCount - 1U
  );
  uint8_t clearBuffer[17] = {SSD13XX_DATA_CONTROL};
  uint8_t blocks = 8U * pageCount;
  for (uint8_t i = 0U; (i < blocks) && (status == SUCCESS); i++) {
    status = I2C_Master_Send(
      device->I2C,
      device->Address,
      clearBuffer,
      sizeof(clearBuffer)
    );
  }
  return (status);
}




// -------------------------------------------------------------
static ErrorStatus ssd13xx_WriteCharacter(
  SSD13xx_TypeDef* device,
  uint8_t character
) {
  uint8_t glyphBuffer[SSD13XX_MAX_GLYPH_BYTES + 1U];
  uint8_t glyphLength;
  uint8_t pageCount;
  uint8_t glyphIndex = ssd13xx_FontIndex(character);

  glyphBuffer[0] = SSD13XX_DATA_CONTROL;
  if (device->Font == SSD13XX_FONT_10X14) {
    glyphLength = sizeof(font_dot_10x14_t);
    pageCount = 2U;
    for (uint8_t i = 0U; i < glyphLength; i++) {
      glyphBuffer[i + 1U] = font_dot_10x14[glyphIndex][i];
    }
  } else {
    glyphLength = sizeof(font_dot_5x7_t);
    pageCount = 1U;
    for (uint8_t i = 0U; i < glyphLength; i++) {
      glyphBuffer[i + 1U] = font_dot_5x7[glyphIndex][i];
    }
  }

  uint8_t glyphWidth = glyphLength / pageCount;
  uint8_t firstColumn = displayColumn * glyphWidth;
  uint8_t firstPage = displayRow * pageCount;
  ErrorStatus status = ssd13xx_SetWindow(
    device,
    (pageCount == 2U) ? 0x01U : 0x00U,
    firstColumn,
    firstColumn + glyphWidth - 1U,
    firstPage,
    firstPage + pageCount - 1U
  );
  if (status != SUCCESS) return (ERROR);
  return I2C_Master_Send(
    device->I2C,
    device->Address,
    glyphBuffer,
    glyphLength + 1U
  );
}




// -------------------------------------------------------------
static uint8_t ssd13xx_FontIndex(uint8_t character) {
  if (character == FONT_DEGREE_CHARACTER) return (FONT_CHARACTER_COUNT - 1U);
  if ((character < FONT_FIRST_CHARACTER)
      || (character > FONT_LAST_CHARACTER)) {
    return ('?' - FONT_FIRST_CHARACTER);
  }
  return character - FONT_FIRST_CHARACTER;
}

#endif /* USE_SSD_DISPLAY */
