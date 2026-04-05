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
#include "stm32f1xx_hal.h"

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
#define VR_Pin GPIO_PIN_5
#define VR_GPIO_Port GPIOA
#define VL_Pin GPIO_PIN_1
#define VL_GPIO_Port GPIOB
#define LL_CTRL_Pin GPIO_PIN_13
#define LL_CTRL_GPIO_Port GPIOB
#define RL_CTRL_Pin GPIO_PIN_14
#define RL_CTRL_GPIO_Port GPIOB
#define LH_CTRL_Pin GPIO_PIN_8
#define LH_CTRL_GPIO_Port GPIOA
#define RH_CTRL_Pin GPIO_PIN_9
#define RH_CTRL_GPIO_Port GPIOA
#define E_SW_Pin GPIO_PIN_3
#define E_SW_GPIO_Port GPIOB
#define E_A_Pin GPIO_PIN_4
#define E_A_GPIO_Port GPIOB
#define E_B_Pin GPIO_PIN_5
#define E_B_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
