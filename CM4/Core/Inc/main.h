/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : Header for main.c file.
 *                   This file contains the common defines of the application.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
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
#include "stm32h7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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
void Error_Handler (void);
void MX_USART1_UART_Init (void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define ULPI_D7_Pin           GPIO_PIN_5
#define ULPI_D7_GPIO_Port     GPIOB
#define HDMI_CEC_Pin          GPIO_PIN_6
#define HDMI_CEC_GPIO_Port    GPIOB
#define OSC32_OUT_Pin         GPIO_PIN_15
#define OSC32_OUT_GPIO_Port   GPIOC
#define OSC32_IN_Pin          GPIO_PIN_14
#define OSC32_IN_GPIO_Port    GPIOC
#define SPDIF_RX0_Pin         GPIO_PIN_7
#define SPDIF_RX0_GPIO_Port   GPIOD
#define SAI1_SCK_A_Pin        GPIO_PIN_5
#define SAI1_SCK_A_GPIO_Port  GPIOE
#define SAI1_FS_A_Pin         GPIO_PIN_4
#define SAI1_FS_A_GPIO_Port   GPIOE
#define SAI1_SD_B_Pin         GPIO_PIN_3
#define SAI1_SD_B_GPIO_Port   GPIOE
// #define STLINK_TX_Pin GPIO_PIN_10
// #define STLINK_TX_GPIO_Port GPIOA
// #define STLINK_RX_Pin GPIO_PIN_9
// #define STLINK_RX_GPIO_Port GPIOA
#define B2_Pin                GPIO_PIN_13
#define B2_GPIO_Port          GPIOC
#define SAI1_SD_A_Pin         GPIO_PIN_6
#define SAI1_SD_A_GPIO_Port   GPIOE
#define CEC_CK_MCO1_Pin       GPIO_PIN_8
#define CEC_CK_MCO1_GPIO_Port GPIOA
#define ULPI_DIR_Pin          GPIO_PIN_11
#define ULPI_DIR_GPIO_Port    GPIOI
#define SAI1_MCLK_A_Pin       GPIO_PIN_7
#define SAI1_MCLK_A_GPIO_Port GPIOG
#define OSC_OUT_Pin           GPIO_PIN_1
#define OSC_OUT_GPIO_Port     GPIOH
#define OSC_IN_Pin            GPIO_PIN_0
#define OSC_IN_GPIO_Port      GPIOH
#define ARD_D13_Pin           GPIO_PIN_0
#define ARD_D13_GPIO_Port     GPIOK
#define ARD_D10_Pin           GPIO_PIN_1
#define ARD_D10_GPIO_Port     GPIOK
#define ARD_D12_Pin           GPIO_PIN_11
#define ARD_D12_GPIO_Port     GPIOJ
#define ULPI_STP_Pin          GPIO_PIN_0
#define ULPI_STP_GPIO_Port    GPIOC
#define ARD_D11_Pin           GPIO_PIN_10
#define ARD_D11_GPIO_Port     GPIOJ
#define ULPI_NXT_Pin          GPIO_PIN_4
#define ULPI_NXT_GPIO_Port    GPIOH
#define ULPI_D3_Pin           GPIO_PIN_10
#define ULPI_D3_GPIO_Port     GPIOB
#define ULPI_D4_Pin           GPIO_PIN_11
#define ULPI_D4_GPIO_Port     GPIOB
#define ULPI_CK_Pin           GPIO_PIN_5
#define ULPI_CK_GPIO_Port     GPIOA
#define ULPI_D2_Pin           GPIO_PIN_1
#define ULPI_D2_GPIO_Port     GPIOB
#define ULPI_D5_Pin           GPIO_PIN_12
#define ULPI_D5_GPIO_Port     GPIOB
#define ULPI_D0_Pin           GPIO_PIN_3
#define ULPI_D0_GPIO_Port     GPIOA
#define ULPI_D1_Pin           GPIO_PIN_0
#define ULPI_D1_GPIO_Port     GPIOB
#define ULPI_D6_Pin           GPIO_PIN_13
#define ULPI_D6_GPIO_Port     GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
