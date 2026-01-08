/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : tcp_echo_server.h
  * @brief          : TCP Echo Server header file
  ******************************************************************************
  * @attention
  *
  * TCP Echo Server with detailed logging
  * - Static IP: 192.168.1.30
  * - Port: 7 (standard echo port)
  * - Logs: Connection, Data received, Disconnection
  *
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __TCP_ECHO_SERVER_H__
#define __TCP_ECHO_SERVER_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "lwip/tcp.h"

/* Exported defines ----------------------------------------------------------*/
#define ECHO_SERVER_PORT  7

/* Exported functions prototypes ---------------------------------------------*/
void tcp_echo_server_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __TCP_ECHO_SERVER_H__ */
