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
  * - Port: 8080
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
#define ECHO_SERVER_PORT  8080

/* Exported functions prototypes ---------------------------------------------*/
void tcp_echo_server_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __TCP_ECHO_SERVER_H__ */
