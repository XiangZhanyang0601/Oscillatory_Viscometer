/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "task1_measurement.h"
#include "task2_calibration.h"
#include "task3_pulse.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
SysWorkMode system_mode = MODE_MEASUREMENT;
/* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> DRV8833 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */
DRV_HandleTypeDef hdrv1;
/* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> LDC1101 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */
LDC_HandleTypeDef hldc1;
/* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> LED <<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */
LED_Handle_t hled1;
LED_Handle_t hled2;
/* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Button <<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */
volatile bool button_scan_pending = false;
/* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> LED <<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */
volatile bool led_breath_enabled = false;
/* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> ADC <<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */
uint16_t adc_buf[2][ADC_BUF_LEN]                  = {0};
uint8_t UART6_tx_packet[2][ADC_UART_PACKET_BYTES] = {0};
volatile uint8_t UART6_sending_index              = UART6_TX_INDEX_NONE;
volatile uint8_t UART6_pending_index              = UART6_TX_INDEX_NONE;
volatile uint8_t UART6_tx_busy                    = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_SPI2_Init();
  MX_TIM2_Init();
  MX_USART2_UART_Init();
  MX_USART3_UART_Init();
  MX_TIM1_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_ADC1_Init();
  MX_USART6_UART_Init();
  MX_TIM5_Init();
  /* USER CODE BEGIN 2 */
    //*------------- Hardware Initialization -------------*//
    buttons_init();
    HAL_TIM_Base_Start_IT(&htim3);

    OLED_Init();
    OLED_Clear();
    OLED_Display_On();

    LED_Init(&hled1, &LED1_hardware);
    LED_Init(&hled2, &LED2_hardware);

    if (BSP_DRV_Init(&hdrv1, &DRV1_hardware) == DEVICE_OK)
        UART2_Log("INFO", __FUNCTION__, __LINE__, "DRV8833 Initialization Done...");
    if (ldc1101_init(&hldc1, &LDC1_hardware) != DEVICE_OK) {
        UART2_Log("ERROR", __FUNCTION__, __LINE__, "LDC1101 Initialization Failed...");
    } else {
        UART2_Log("INFO", __FUNCTION__, __LINE__, "LDC1101 Initialization Done...");
        if (ldc1101_writeConfig(&hldc1, &LDC_LHR_cfg, LDC_LHR_cfg_size) == DEVICE_OK) {
            UART2_Log("INFO", __FUNCTION__, __LINE__, "LDC1101 Configuration Done...");
            snprintf(UART2_TX_buffer, sizeof(UART2_TX_buffer), "LDC1101 sample rate: %.3f kSPS (RCOUNT=0x%04X)",
                     ldc1101_getLHRSampleRate(&hldc1) / 1000.0f, ldc1101_getLHRRCount(&hldc1));
            UART2_Log("TRACE", __FUNCTION__, __LINE__, UART2_TX_buffer);
        } else {
            UART2_Log("ERROR", __FUNCTION__, __LINE__, "LDC1101 Configuration Failed...");
        }
    }

    HAL_UART_Receive_DMA(&huart2, (uint8_t *)UART2_RX_DMA_buffer[UART2_RX_activeBuffer], MSG_LEN);
    __HAL_UART_ENABLE_IT(&huart2, UART_IT_IDLE);

    //*------------- Service Initialization -------------*//
    Oscillate_Init();
    LHRSample_Init();

    //*------------- Application Initialization -------------*//
    switch (system_mode) {
        case MODE_MEASUREMENT:
            Task1_Measurement_Init();
            break;
        case MODE_CALIBRATION:
            Task2_Calibration_Init();
            break;
        case MODE_PULSE_FEEDBACK:
            Task3_Pulse_Init();
            break;
        default:
            break;
    }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
    while (1) {
        if (system_mode == MODE_MEASUREMENT) {
            Task1_Measurement_Run();
        } else if (system_mode == MODE_CALIBRATION) {
            Task2_Calibration_Run();
        } else if (system_mode == MODE_PULSE_FEEDBACK) {
            Task3_Pulse_Run();
        }
        if (OLED_GetUpdatePending()) {
            OLED_Update(system_mode);
        }
        if (button_scan_pending) {
            button_scan_pending = false;
            button_ticks();
        }
        /* UART2 Transmission */
        if (UART2_TX_send) {
            HAL_UART_Transmit(&huart2, (uint8_t *)UART2_TX_buffer, strlen(UART2_TX_buffer), 0x0100);
            UART2_TX_send = false;
            memset(UART2_TX_buffer, 0, MSG_LEN);
        }
    }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1) {
    }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
