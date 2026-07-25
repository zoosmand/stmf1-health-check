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
#define TCP_RESPONSE_SIZE       128U

static uint8_t commandBuffer[TCP_COMMAND_BUFFER_SIZE];
static uint16_t commandLength;

static void tcpCommandService_Task(void*);
static void tcpCommandService_Receive(void);
static void tcpCommandService_ProcessInput(void);
static void tcpCommandService_ProcessCommand(const uint8_t*, uint16_t);
static ErrorStatus tcpCommandService_Send(const char*);
static ErrorStatus tcpCommandService_ParseSensorNumber(
  const uint8_t*,
  uint16_t,
  uint16_t,
  uint8_t*
);
static const char* tcpCommandService_ModelName(SensorModel_TypeDef);
static void tcpCommandService_AppendTemperature(char*, size_t, size_t*, int16_t);
static void tcpCommandService_AppendPressure(char*, size_t, size_t*, uint32_t);
static void tcpCommandService_AppendHumidity(char*, size_t, size_t*, uint32_t);




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
        printf("TCP connection\n");
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
  char response[TCP_RESPONSE_SIZE];
  size_t used = 0U;

  /* Preserve the original command as an alias returning all temperatures. */
  if ((length == 8U) && (memcmp(command, "get_tmpr", 8U) == 0)) {
    SensorCounts_TypeDef counts;
    TemperatureSensorService_GetCounts(&counts);
    used = (size_t)snprintf(response, sizeof(response), "OK");
    for (uint8_t i = 1U; i <= counts.temperature; i++) {
      SensorSnapshot_TypeDef snapshot;
      if ((TemperatureSensorService_GetByCapability(
             SENSOR_CAPABILITY_TEMPERATURE,
             i,
             &snapshot
           ) != SUCCESS)
          || (snapshot.dataValid != pdTRUE)) {
        (void)tcpCommandService_Send("ERR temperature_unavailable\r\n");
        return;
      }
      response[used++] = ' ';
      tcpCommandService_AppendTemperature(
        response,
        sizeof(response),
        &used,
        snapshot.temperature
      );
    }
    if (counts.temperature == 0U) {
      (void)tcpCommandService_Send("ERR temperature_unavailable\r\n");
      return;
    }
    if (used < (sizeof(response) - 2U)) {
      response[used++] = '\r';
      response[used++] = '\n';
      response[used] = '\0';
    }
    (void)tcpCommandService_Send(response);
    return;
  }

  if ((length == 11U) && (memcmp(command, "get_sensors", 11U) == 0)) {
    SensorCounts_TypeDef counts;
    TemperatureSensorService_GetCounts(&counts);
    (void)snprintf(
      response,
      sizeof(response),
      "OK t:%u,h:%u,p:%u,all:%u\r\n",
      counts.temperature,
      counts.humidity,
      counts.pressure,
      counts.all
    );
    (void) tcpCommandService_Send(response);
    return;
  }

  typedef enum {
    TCP_SENSOR_COMMAND_NONE = 0U,
    TCP_SENSOR_COMMAND_MODEL,
    TCP_SENSOR_COMMAND_ALL,
    TCP_SENSOR_COMMAND_TEMPERATURE,
    TCP_SENSOR_COMMAND_PRESSURE,
    TCP_SENSOR_COMMAND_HUMIDITY
  } TcpSensorCommand_TypeDef;

  TcpSensorCommand_TypeDef commandType = TCP_SENSOR_COMMAND_NONE;
  uint16_t prefixLength = 0U;
  if ((length > 10U) && (memcmp(command, "get_model_", 10U) == 0)) {
    commandType = TCP_SENSOR_COMMAND_MODEL;
    prefixLength = 10U;
  } else if ((length > 8U) && (memcmp(command, "get_all_", 8U) == 0)) {
    commandType = TCP_SENSOR_COMMAND_ALL;
    prefixLength = 8U;
  } else if ((length > 6U) && (memcmp(command, "get_t_", 6U) == 0)) {
    commandType = TCP_SENSOR_COMMAND_TEMPERATURE;
    prefixLength = 6U;
  } else if ((length > 6U) && (memcmp(command, "get_p_", 6U) == 0)) {
    commandType = TCP_SENSOR_COMMAND_PRESSURE;
    prefixLength = 6U;
  } else if ((length > 6U) && (memcmp(command, "get_h_", 6U) == 0)) {
    commandType = TCP_SENSOR_COMMAND_HUMIDITY;
    prefixLength = 6U;
  }

  if (commandType == TCP_SENSOR_COMMAND_NONE) {
    (void) tcpCommandService_Send("ERR unknown_command\r\n");
    return;
  }

  uint8_t sensorNumber;
  if (tcpCommandService_ParseSensorNumber(
        command,
        length,
        prefixLength,
        &sensorNumber
      ) != SUCCESS) {
    (void) tcpCommandService_Send("ERR invalid_sensor_number\r\n");
    return;
  }

  SensorSnapshot_TypeDef snapshot;
  ErrorStatus lookupStatus;
  if ((commandType == TCP_SENSOR_COMMAND_MODEL)
      || (commandType == TCP_SENSOR_COMMAND_ALL)) {
    lookupStatus = TemperatureSensorService_GetSnapshot(sensorNumber, &snapshot);
  } else {
    SensorCapability_TypeDef capability = SENSOR_CAPABILITY_TEMPERATURE;
    if (commandType == TCP_SENSOR_COMMAND_PRESSURE) {
      capability = SENSOR_CAPABILITY_PRESSURE;
    } else if (commandType == TCP_SENSOR_COMMAND_HUMIDITY) {
      capability = SENSOR_CAPABILITY_HUMIDITY;
    }
    lookupStatus = TemperatureSensorService_GetByCapability(
      capability,
      sensorNumber,
      &snapshot
    );
  }
  if (lookupStatus != SUCCESS) {
    (void) tcpCommandService_Send("ERR sensor_not_found\r\n");
    return;
  }

  if (commandType == TCP_SENSOR_COMMAND_MODEL) {
    (void)snprintf(
      response,
      sizeof(response),
      "OK %s\r\n",
      tcpCommandService_ModelName(snapshot.model)
    );
    (void)tcpCommandService_Send(response);
    return;
  }
  if (snapshot.dataValid != pdTRUE) {
    (void)tcpCommandService_Send("ERR measurement_unavailable\r\n");
    return;
  }

  used = (size_t)snprintf(response, sizeof(response), "OK");
  if ((commandType == TCP_SENSOR_COMMAND_ALL)
      || (commandType == TCP_SENSOR_COMMAND_TEMPERATURE)) {
    if (commandType == TCP_SENSOR_COMMAND_ALL) {
      used += (size_t)snprintf(
        &response[used],
        sizeof(response) - used,
        " model:%s,t:",
        tcpCommandService_ModelName(snapshot.model)
      );
    } else {
      response[used++] = ' ';
    }
    tcpCommandService_AppendTemperature(
      response,
      sizeof(response),
      &used,
      snapshot.temperature
    );
  }
  if ((commandType == TCP_SENSOR_COMMAND_ALL)
      && ((snapshot.capabilities & SENSOR_CAPABILITY_PRESSURE) != 0U)) {
    used += (size_t)snprintf(&response[used], sizeof(response) - used, ",p:");
    tcpCommandService_AppendPressure(response, sizeof(response), &used, snapshot.pressure);
  } else if (commandType == TCP_SENSOR_COMMAND_PRESSURE) {
    response[used++] = ' ';
    tcpCommandService_AppendPressure(response, sizeof(response), &used, snapshot.pressure);
  }
  if ((commandType == TCP_SENSOR_COMMAND_ALL)
      && ((snapshot.capabilities & SENSOR_CAPABILITY_HUMIDITY) != 0U)) {
    used += (size_t)snprintf(&response[used], sizeof(response) - used, ",h:");
    tcpCommandService_AppendHumidity(response, sizeof(response), &used, snapshot.humidity);
  } else if (commandType == TCP_SENSOR_COMMAND_HUMIDITY) {
    response[used++] = ' ';
    tcpCommandService_AppendHumidity(response, sizeof(response), &used, snapshot.humidity);
  }
  if (used < (sizeof(response) - 2U)) {
    response[used++] = '\r';
    response[used++] = '\n';
    response[used] = '\0';
  }
  (void)tcpCommandService_Send(response);
}




// -------------------------------------------------------------
static ErrorStatus tcpCommandService_ParseSensorNumber(
  const uint8_t* command,
  uint16_t length,
  uint16_t prefixLength,
  uint8_t* sensorNumber
) {
  if ((sensorNumber == NULL) || (prefixLength >= length)) return (ERROR);
  uint16_t value = 0U;
  for (uint16_t i = prefixLength; i < length; i++) {
    if ((command[i] < '0') || (command[i] > '9')) return (ERROR);
    value = (uint16_t)(value * 10U + (command[i] - '0'));
    if (value > 255U) return (ERROR);
  }
  if (value == 0U) return (ERROR);
  *sensorNumber = (uint8_t)value;
  return (SUCCESS);
}




// -------------------------------------------------------------
static const char* tcpCommandService_ModelName(SensorModel_TypeDef model) {
  switch (model) {
    case SENSOR_MODEL_DS18B20: return ("DS18B20");
    case SENSOR_MODEL_BMP280:  return ("BMP280");
    case SENSOR_MODEL_BME280:  return ("BME280");
    case SENSOR_MODEL_BME680:  return ("BME680");
    default:                   return ("unknown");
  }
}




// -------------------------------------------------------------
static void tcpCommandService_AppendPressure(
  char* response,
  size_t capacity,
  size_t* used,
  uint32_t pressure
) {
  int written = snprintf(
    &response[*used],
    capacity - *used,
    "%lu",
    (unsigned long)pressure
  );
  if (written > 0) {
    size_t remaining = capacity - *used;
    size_t appended = (size_t)written;
    *used += (appended < remaining) ? appended : (remaining - 1U);
  }
}




// -------------------------------------------------------------
static void tcpCommandService_AppendHumidity(
  char* response,
  size_t capacity,
  size_t* used,
  uint32_t humidity
) {
  int written = snprintf(
    &response[*used],
    capacity - *used,
    "%lu.%03lu",
    (unsigned long)(humidity / 1000U),
    (unsigned long)(humidity % 1000U)
  );
  if (written > 0) {
    size_t remaining = capacity - *used;
    size_t appended = (size_t)written;
    *used += (appended < remaining) ? appended : (remaining - 1U);
  }
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
    "%s%lu.%02lu",
    (centiDegrees < 0) ? "-" : "",
    (unsigned long)(magnitude / 100U),
    (unsigned long)(magnitude % 100U)
  );
  if (written > 0) {
    size_t remaining = capacity - *used;
    size_t appended = (size_t)written;
    *used += (appended < remaining) ? appended : (remaining - 1U);
  }
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

  setSn_IR(TCP_COMMAND_SOCKET, Sn_IR_SENDOK);
  (void) disconnect(TCP_COMMAND_SOCKET);
  commandLength = 0U;
  return (SUCCESS);
}
