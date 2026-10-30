/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : Header for main.c file.
 *                   This file contains the common defines of the application.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include "ldc1101_driver.h"
#include "drv8833_driver.h"
#include "led.h"
#include "oled_driver.h"
#include "multi_button.h"

#include "command.h"

#include "utility.h"

#include "usart2.h"
#include "usart3.h"
#include "oled.h"

#include "my_button.h"

#include "oscillate.h"
#include "lhr_sample.h"

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/

/* USER CODE BEGIN Private defines */
extern SysWorkMode system_mode;
/* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Pulse Feedback <<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */
extern volatile bool pulse_active;
extern volatile uint8_t pulse_feedback_trigger;
extern volatile bool pulse_feedback_busy;
extern volatile uint32_t pulse_feedback_frame_cnt;

extern volatile bool button_scan_pending;
/* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> ADC <<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */
#define ADC_BUF_LEN           256
#define ADC_DMA_BUF_LEN       (ADC_BUF_LEN * 2U)
#define ADC_UART_TX_BYTES     (ADC_BUF_LEN * sizeof(uint16_t))
#define ADC_UART_PACKET_BYTES (ADC_UART_TX_BYTES + 2U)
#define UART6_FRAME_HEAD      0xAAU
#define UART6_FRAME_TAIL      0xFFU
#define UART6_TX_INDEX_NONE   0xFFU
extern uint16_t adc_buf[2][ADC_BUF_LEN];
extern uint8_t UART6_tx_packet[2][ADC_UART_PACKET_BYTES];
extern volatile uint8_t UART6_sending_index;
extern volatile uint8_t UART6_pending_index;
extern volatile uint8_t UART6_tx_busy;
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
