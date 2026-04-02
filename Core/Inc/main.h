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
#include "stm32g4xx_hal.h"

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

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define CH1_DATA_Pin GPIO_PIN_0
#define CH1_DATA_GPIO_Port GPIOA
#define CH2_DATA_Pin GPIO_PIN_1
#define CH2_DATA_GPIO_Port GPIOA
#define CH3_DATA_Pin GPIO_PIN_2
#define CH3_DATA_GPIO_Port GPIOA
#define CH4_DATA_Pin GPIO_PIN_3
#define CH4_DATA_GPIO_Port GPIOA
#define CH1_EN_Pin GPIO_PIN_6
#define CH1_EN_GPIO_Port GPIOA
#define CH2_EN_Pin GPIO_PIN_7
#define CH2_EN_GPIO_Port GPIOA
#define LED_PWR_EN_Pin GPIO_PIN_1
#define LED_PWR_EN_GPIO_Port GPIOB
#define LED_DATA_OE_Pin GPIO_PIN_2
#define LED_DATA_OE_GPIO_Port GPIOB
#define LED_HANDLE_PWM_Pin GPIO_PIN_6
#define LED_HANDLE_PWM_GPIO_Port GPIOC
#define CH4_EN_Pin GPIO_PIN_8
#define CH4_EN_GPIO_Port GPIOA
#define CH3_EN_Pin GPIO_PIN_9
#define CH3_EN_GPIO_Port GPIOA
#define FDCAN1_STBY_Pin GPIO_PIN_6
#define FDCAN1_STBY_GPIO_Port GPIOB
#define FDCAN1_WAKEUP_Pin GPIO_PIN_7
#define FDCAN1_WAKEUP_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* Демо режим: автоматическое переключение тем каждые 20 секунд (только для master) */
/* Установите в 0 для продакшн режима (управление только через CAN) */
#ifndef DEMO_MODE
#define DEMO_MODE  0  /* 0 = production mode, 1 = demo mode */
#endif

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
