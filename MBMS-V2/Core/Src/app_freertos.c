/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : app_freertos.c
  * Description        : FreeRTOS applicative file
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

/* Includes ------------------------------------------------------------------*/
#include "app_freertos.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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
/* USER CODE BEGIN Variables */
//Task Handles
osThreadId_t canTxTaskHandle;
const osThreadAttr_t canTxTask_attributes = {
    .name       = "canTxTask",
    .priority   = (osPriority_t) osPriorityNormal,
    .stack_size = 256 * 4
  };
osThreadId_t canRxTaskHandle;
const osThreadAttr_t canRxTask_attributes = {
	.name 		= "canRxTask",
	.priority   = (osPriority_t) osPriorityNormal,
    .stack_size = 256 * 4
  };
osThreadId_t ContactorsTaskHandle;
const osThreadAttr_t Contactors_attributes = {
	.name 		= "ContactorsTask",
	.priority   = (osPriority_t) osPriorityNormal,
    .stack_size = 256 * 4
  };
osThreadId_t BatteryTaskHandle;
const osThreadAttr_t Battery_attributes = {
	.name 		= "BatteryTask",
	.priority   = (osPriority_t) osPriorityNormal,
    .stack_size = 256 * 4
  };

//Queue Handles
osMessageQueueId_t canTxQueueHandle;
const osMessageQueueAttr_t canTxQueue_attributes = {
	  .name = "canTxQueue"
	};

osMessageQueueId_t canRxQueueHandle;
const osMessageQueueAttr_t canRxQueue_attributes = {
	  .name = "canRxQueue"
	};

osMessageQueueId_t ContactorQueueHandle;
const osMessageQueueAttr_t contactors_attributes = {
	  .name = "contactorQueue"
	};

osMessageQueueId_t BatteryQueueHandle;
const osMessageQueueAttr_t battery_attributes = {
	  .name = "batteryQueue"
	};

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 128 * 4
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */


	canTxQueueHandle = osMessageQueueNew(
	    8,                     // max number of messages
	    sizeof(CANmsg),        // size of each message
	    &canTxQueue_attributes // attributes (or NULL)
	);

	canRxQueueHandle = osMessageQueueNew(
			    8,                     // max number of messages
			    sizeof(CANmsg),        // size of each message
			    &canRxQueue_attributes // attributes (or NULL)
			);

	ContactorQueueHandle = osMessageQueueNew(
				    8,                     // max number of messages
				    sizeof(CANmsg),        // size of each message
				    &contactors_attributes // attributes (or NULL)
				);

	BatteryQueueHandle = osMessageQueueNew(
				    8,                     // max number of messages
				    sizeof(CANmsg),        // size of each message
				    &battery_attributes // attributes (or NULL)
				);

  /* USER CODE END RTOS_QUEUES */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */

  canTxTaskHandle = osThreadNew(CAN_Tx_Task, NULL, &canTxTask_attributes);
  canRxTaskHandle = osThreadNew(CAN_Rx_Task, NULL, &canRxTask_attributes);
  //ContactorsTaskHandle = osThreadNew(ContactorsTask, NULL, &Contactors_attributes);
  //BatteryTaskHandle = osThreadNew(BatteryTask, NULL, &Battery_attributes);
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}
/* USER CODE BEGIN Header_StartDefaultTask */
/**
* @brief Function implementing the defaultTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN defaultTask */
  /* Infinite loop */
(void)argument;


  for (;;)
  {

	  osDelay(1000);
  }

  /* USER CODE END defaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

