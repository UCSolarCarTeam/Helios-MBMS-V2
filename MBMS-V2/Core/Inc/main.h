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
#include "stm32h5xx_hal.h"

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
#define MPS_Pin GPIO_PIN_2
#define MPS_GPIO_Port GPIOE
#define ESD_Pin GPIO_PIN_3
#define ESD_GPIO_Port GPIOE
#define DEBUG_UART4_TX_Pin GPIO_PIN_0
#define DEBUG_UART4_TX_GPIO_Port GPIOA
#define DEBUG_UART4_RX_Pin GPIO_PIN_1
#define DEBUG_UART4_RX_GPIO_Port GPIOA
#define DCDC1_Fault_Pin GPIO_PIN_2
#define DCDC1_Fault_GPIO_Port GPIOB
#define _12V_Critical_Fault_Pin GPIO_PIN_7
#define _12V_Critical_Fault_GPIO_Port GPIOE
#define _12V_Charger_Fault_Pin GPIO_PIN_8
#define _12V_Charger_Fault_GPIO_Port GPIOE
#define _12V_Critical_OC_Pin GPIO_PIN_9
#define _12V_Critical_OC_GPIO_Port GPIOE
#define DCDC1_EN_Pin GPIO_PIN_11
#define DCDC1_EN_GPIO_Port GPIOE
#define Strobe_EN_Pin GPIO_PIN_9
#define Strobe_EN_GPIO_Port GPIOC
#define _14V_Charge_EN_Pin GPIO_PIN_8
#define _14V_Charge_EN_GPIO_Port GPIOA
#define Abatt_EN_Pin GPIO_PIN_9
#define Abatt_EN_GPIO_Port GPIOA
#define EVCC_12V_Sw_Pin GPIO_PIN_2
#define EVCC_12V_Sw_GPIO_Port GPIOD
#define Charge_Enable_Pin GPIO_PIN_4
#define Charge_Enable_GPIO_Port GPIOD
#define Discharge_Enable_Pin GPIO_PIN_5
#define Discharge_Enable_GPIO_Port GPIOD
#define Charge_Safety_Pin GPIO_PIN_6
#define Charge_Safety_GPIO_Port GPIOD
#define Common_CNTR_Aux_Pin GPIO_PIN_6
#define Common_CNTR_Aux_GPIO_Port GPIOB
#define Main_CNTR_Aux_Pin GPIO_PIN_7
#define Main_CNTR_Aux_GPIO_Port GPIOB
#define _12V_CAN_Pin GPIO_PIN_9
#define _12V_CAN_GPIO_Port GPIOB
#define _12V_Critical_Pin GPIO_PIN_0
#define _12V_Critical_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
