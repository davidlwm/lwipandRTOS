/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : retarget.h
  * @brief          : Printf retarget header file
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __RETARGET_H__
#define __RETARGET_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_uart.h"

/* Exported variables --------------------------------------------------------*/
extern UART_HandleTypeDef huart1;

/* Exported functions prototypes ---------------------------------------------*/
void MX_USART1_UART_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* __RETARGET_H__ */
