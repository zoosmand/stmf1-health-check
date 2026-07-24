/**
  ******************************************************************************
  * @file           : tcp_service.c
  * @brief          : TCP command service on port 5005.
  ******************************************************************************
  */

#include "main.h"
#include "tcp_service.h"
#include "socket.h"
#include <string.h>

#define TCP_COMMAND_SOCKET      2U
#define TCP_COMMAND_PORT        5005U
#define TCP_COMMAND_BUFFER_SIZE 64U
#define TCP_RESPONSE_SIZE       64U
#define TCP_MAX_TEMPERATURES    2U

static uint8_t commandBuffer[TCP_COMMAND_BUFFER_SIZE];
static uint16_t commandLength;

static void tcpCommandService_Task(void*);
static void tcpCommandService_Receive(void);
static void tcpCommandService_ProcessInput(void);
static void tcpCommandService_ProcessCommand(const uint8_t*, uint16_t);
static ErrorStatus tcpCommandService_Send(const char*);
static void tcpCommandService_AppendTemperature(char*, size_t, size_t*, int16_t);




// -------------------------------------------------------------
void TcpCommandService_Init(void) {
  static StaticTask_t taskControlBlock;
  static StackType_t taskStack[512];

  (void) xTaskCreateStatic(
    tcpCommandService_Task,
    "TCP Commands",
    512,
    NULL,
    configMAX_PRIORITIES - 3U,
    taskStack,
    &taskControlBlock
  );
}




// -------------------------------------------------------------
static void tcpCommandService_Task(void* parameters) {
  (void) parameters;

  while (1) {
    if (SPI_Enable(SPI1) == SUCCESS) {
      TcpCommandService_Run();
      (void) SPI_Disable(SPI1);
    }
    vTaskDelay(1U);
  }
}




// -------------------------------------------------------------
void TcpCommandService_Run(void) {
  switch (getSn_SR(TCP_COMMAND_SOCKET)) {
    case SOCK_ESTABLISHED:
      if ((getSn_IR(TCP_COMMAND_SOCKET) & Sn_IR_CON) != 0U) {
        setSn_IR(TCP_COMMAND_SOCKET, Sn_IR_CON);
        commandLength = 0U;
      }

      tcpCommandService_Receive();
      break;

    case SOCK_CLOSE_WAIT:
      if (getSn_RX_RSR(TCP_COMMAND_SOCKET) > 0U) {
        tcpCommandService_Receive();
      }
      if (getSn_SR(TCP_COMMAND_SOCKET) == SOCK_CLOSE_WAIT) {
        (void) disconnect(TCP_COMMAND_SOCKET);
        commandLength = 0U;
      }
      break;

    case SOCK_INIT:
      if (listen(TCP_COMMAND_SOCKET) != SOCK_OK) {
        (void) close(TCP_COMMAND_SOCKET);
      }
      break;

    case SOCK_CLOSED:
      commandLength = 0U;
      if (socket(TCP_COMMAND_SOCKET, Sn_MR_TCP, TCP_COMMAND_PORT, 0x00) != TCP_COMMAND_SOCKET) {
        (void) close(TCP_COMMAND_SOCKET);
      }
      break;

    default:
      break;
  }
}




// -------------------------------------------------------------
static void tcpCommandService_Receive(void) {
  uint16_t receivedSize = getSn_RX_RSR(TCP_COMMAND_SOCKET);
  if (receivedSize == 0U) return;

  uint16_t available = TCP_COMMAND_BUFFER_SIZE - commandLength;
  uint16_t readSize = (receivedSize < available) ? receivedSize : available;
  if (readSize == 0U) {
    (void) tcpCommandService_Send("ERR command_too_long\r\n");
    commandLength = 0U;
    return;
  }

  int32_t result = recv(TCP_COMMAND_SOCKET, &commandBuffer[commandLength], readSize);
  if (result <= 0) {
    (void) close(TCP_COMMAND_SOCKET);
    commandLength = 0U;
    return;
  }

  commandLength += (uint16_t)result;
  tcpCommandService_ProcessInput();
}




// -------------------------------------------------------------
static void tcpCommandService_ProcessInput(void) {
  uint16_t commandStart = 0U;

  for (uint16_t i = 0U; i < commandLength; i++) {
    if ((commandBuffer[i] == '\r') || (commandBuffer[i] == '\n')) {
      if (i > commandStart) {
        tcpCommandService_ProcessCommand(&commandBuffer[commandStart], i - commandStart);
        commandLength = 0U;
        return;
      }
      commandStart = i + 1U;
    }
  }

  if (commandStart > 0U) {
    uint16_t remaining = commandLength - commandStart;
    memmove(commandBuffer, &commandBuffer[commandStart], remaining);
    commandLength = remaining;
  }

  /* Also accept the complete example command without a line terminator. */
  if ((commandLength == 8U) && (memcmp(commandBuffer, "get_tmpr", 8U) == 0)) {
    tcpCommandService_ProcessCommand(commandBuffer, commandLength);
    commandLength = 0U;
  } else if (commandLength == TCP_COMMAND_BUFFER_SIZE) {
    (void) tcpCommandService_Send("ERR command_too_long\r\n");
    commandLength = 0U;
  }
}




// -------------------------------------------------------------
static void tcpCommandService_ProcessCommand(const uint8_t* command, uint16_t length) {
  if ((length == 8U) && (memcmp(command, "get_tmpr", 8U) == 0)) {
    int16_t temperatures[TCP_MAX_TEMPERATURES];
    uint8_t count = 0U;
    char response[TCP_RESPONSE_SIZE];
    size_t used = 0U;

    if (DS18B20_GetRecentTemperatures(temperatures, TCP_MAX_TEMPERATURES, &count) != SUCCESS) {
      (void) tcpCommandService_Send("ERR temperature_unavailable\r\n");
      return;
    }

    used = (size_t)snprintf(response, sizeof(response), "OK");
    for (uint8_t i = 0U; (i < count) && (used < sizeof(response)); i++) {
      tcpCommandService_AppendTemperature(response, sizeof(response), &used, temperatures[i]);
    }
    if (used < (sizeof(response) - 2U)) {
      response[used++] = '\r';
      response[used++] = '\n';
      response[used] = '\0';
    }
    (void) tcpCommandService_Send(response);
    return;
  }

  (void) tcpCommandService_Send("ERR unknown_command\r\n");
}




// -------------------------------------------------------------
static ErrorStatus tcpCommandService_Send(const char* response) {
  uint16_t length = (uint16_t)strlen(response);
  uint16_t sent = 0U;

  while (sent < length) {
    int32_t result = send(TCP_COMMAND_SOCKET, (uint8_t*)&response[sent], length - sent);
    if (result <= 0) {
      (void) close(TCP_COMMAND_SOCKET);
      return (ERROR);
    }
    sent += (uint16_t)result;
  }

  TickType_t start = xTaskGetTickCount();
  TickType_t timeout = pdMS_TO_TICKS(1000U);
  while ((getSn_IR(TCP_COMMAND_SOCKET) & Sn_IR_SENDOK) == 0U) {
    uint8_t status = getSn_SR(TCP_COMMAND_SOCKET);
    if ((status != SOCK_ESTABLISHED) && (status != SOCK_CLOSE_WAIT)) {
      (void) close(TCP_COMMAND_SOCKET);
      return (ERROR);
    }
    if ((xTaskGetTickCount() - start) >= timeout) {
      (void) close(TCP_COMMAND_SOCKET);
      return (ERROR);
    }
    vTaskDelay(1U);
  }

  (void) disconnect(TCP_COMMAND_SOCKET);
  commandLength = 0U;
  return (SUCCESS);
}




// -------------------------------------------------------------
static void tcpCommandService_AppendTemperature(
  char* response,
  size_t capacity,
  size_t* used,
  int16_t centiDegrees
) {
  uint32_t magnitude = (centiDegrees < 0)
    ? (uint32_t)(-(int32_t)centiDegrees)
    : (uint32_t)centiDegrees;

  int written = snprintf(
    &response[*used],
    capacity - *used,
    " %s%lu.%02lu",
    (centiDegrees < 0) ? "-" : "",
    (unsigned long)(magnitude / 100U),
    (unsigned long)(magnitude % 100U)
  );
  if (written > 0) {
    size_t appended = (size_t)written;
    *used += (appended < (capacity - *used)) ? appended : (capacity - *used - 1U);
  }
}
