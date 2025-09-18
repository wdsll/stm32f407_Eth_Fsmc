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
#include "stm32f4xx_hal.h"

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
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define ADG_I_A0_Pin GPIO_PIN_0
#define ADG_I_A0_GPIO_Port GPIOF
#define ADG_I_A1_Pin GPIO_PIN_1
#define ADG_I_A1_GPIO_Port GPIOF
#define ADG_I_A2_Pin GPIO_PIN_2
#define ADG_I_A2_GPIO_Port GPIOF
#define ADG_FUN_A0_Pin GPIO_PIN_3
#define ADG_FUN_A0_GPIO_Port GPIOF
#define ADG_FUN_A1_Pin GPIO_PIN_4
#define ADG_FUN_A1_GPIO_Port GPIOF
#define ADG_FUN_A2_Pin GPIO_PIN_5
#define ADG_FUN_A2_GPIO_Port GPIOF
#define ADG_R_A0_Pin GPIO_PIN_6
#define ADG_R_A0_GPIO_Port GPIOF
#define ADG_R_A1_Pin GPIO_PIN_7
#define ADG_R_A1_GPIO_Port GPIOF
#define ADG_R_A2_Pin GPIO_PIN_8
#define ADG_R_A2_GPIO_Port GPIOF
#define ADG_R_A3_Pin GPIO_PIN_9
#define ADG_R_A3_GPIO_Port GPIOF
#define ADG_R_A4_Pin GPIO_PIN_10
#define ADG_R_A4_GPIO_Port GPIOF
#define KeyUp_Pin GPIO_PIN_0
#define KeyUp_GPIO_Port GPIOA
#define ETH_RESET_Pin GPIO_PIN_3
#define ETH_RESET_GPIO_Port GPIOA
#define ADG_R_A5_Pin GPIO_PIN_13
#define ADG_R_A5_GPIO_Port GPIOF
#define V_RELAY1_Pin GPIO_PIN_0
#define V_RELAY1_GPIO_Port GPIOG
#define V_RELAY2_Pin GPIO_PIN_1
#define V_RELAY2_GPIO_Port GPIOG
#define SPI1_CS_Pin GPIO_PIN_14
#define SPI1_CS_GPIO_Port GPIOB
#define LCD_BL_Pin GPIO_PIN_15
#define LCD_BL_GPIO_Port GPIOB
#define V_RELAY3_Pin GPIO_PIN_2
#define V_RELAY3_GPIO_Port GPIOG
#define ADC_SYNC_Pin GPIO_PIN_6
#define ADC_SYNC_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
