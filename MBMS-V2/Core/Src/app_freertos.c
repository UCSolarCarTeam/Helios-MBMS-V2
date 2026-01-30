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

//Queue Handles
osMessageQueueId_t canTxQueueHandle;
const osMessageQueueAttr_t canTxQueue_attributes = {
	  .name = "canTxQueue"
	};

osMessageQueueId_t canRxQueueHandle;
const osMessageQueueAttr_t canRxQueue_attributes = {
	  .name = "canRxQueue"
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


  /* USER CODE END RTOS_QUEUES */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */

  canTxTaskHandle = osThreadNew(CAN_Tx_Task, NULL, &canTxTask_attributes);
  canRxTaskHandle = osThreadNew(CAN_Rx_Task, NULL, &canRxTask_attributes);
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

  CANmsg txMsg;
  txMsg.extendedID = 0x100;
  txMsg.ID 		   = 0x100;
  txMsg.DLC		   = FDCAN_DLC_BYTES_8; //8 BYTE PAYLOAD

  txMsg.data[0] = 0x11;
  txMsg.data[1] = 0x22;
  txMsg.data[2] = 0x33;
  txMsg.data[3] = 0x44;
  txMsg.data[4] = 0x55;
  txMsg.data[5] = 0x66;
  txMsg.data[6] = 0x77;
  txMsg.data[7] = 0x88;

  for (;;)
  {
	  osMessageQueuePut(canTxQueueHandle, &txMsg, 0, 0);

	  osDelay(1000);
  }

  /* USER CODE END defaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

