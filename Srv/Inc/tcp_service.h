/**
  ******************************************************************************
  * @file           : tcp_service.h
  * @brief          : TCP command service interface.
  ******************************************************************************
  */

#ifndef __TCP_SERVICE_H
#define __TCP_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
  * @brief Creates the TCP command server task.
  */
void TcpCommandService_Init(void);

/**
  * @brief Advances the non-blocking TCP command server state machine.
  */
void TcpCommandService_Run(void);

#ifdef __cplusplus
}
#endif

#endif /* __TCP_SERVICE_H */
