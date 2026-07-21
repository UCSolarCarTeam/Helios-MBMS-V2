/*
 * can_rx.c
 *
 *  Created on: Jan 4, 2026
 *      Author: helay
 */


#include "can_rx.h"
#include "main.h"
#include "cmsis_os2.h"
#include "CAN.h"

extern FDCAN_HandleTypeDef hfdcan1; //defined in main.c
extern osMessageQueueId_t canRxQueueHandle;
extern osMessageQueueId_t BatteryQueueHandle;
extern osMessageQueueId_t ContactorQueueHandle;

volatile uint32_t g_rx_cb0_hits = 0;
volatile uint32_t g_rx_cb1_hits = 0;
volatile uint32_t messages_got_yay=0;
volatile uint32_t batteryqueuefull=0;
volatile uint32_t contactorqueuefull=0;

volatile uint32_t orion_message_added=0;
volatile uint32_t contactor_msg_ctr = 0;
static void CAN_Rx(void);

void CAN_Rx_Task(void *argument)
{
	(void)argument;
	uint32_t taskTickLastStart = osKernelGetTickCount();
	for(;;)
	{//RTOS tasks run forever. CAN_Rx_Task will sit in this forever loop always checking for new CAN frames.
		CAN_Rx();
	}
}

static void CAN_Rx()
{

	CANmsg msg;

	osStatus_t status = osMessageQueueGet(canRxQueueHandle, &msg, 0, osWaitForever);

	 if(status != osOK)
	 {
		 Error_Handler();

	 //HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
	 }

 	 else
 	 {

 		 messages_got_yay++;

 		 if(msg.extendedID == 0x210) {
			 uint8_t hellooo = 0;
		 }

 		 if(msg.extendedID == PACK_INFO_ID || msg.extendedID == TEMP_INFO_ID || msg.extendedID == CELL_VOLTAGES_ID)
 		 {
 			 status = osMessageQueuePut(BatteryQueueHandle, &msg, 0, 0);

 			 if(status != osOK)
 			 {
 				 batteryqueuefull++;
 			 }

			 else
			 {
				 orion_message_added++;
			 }
 		 }
 		 else if((msg.extendedID & CONTACTOR_MASK) == CONTACTOR_HEARTBEAT)
 		 {
 			contactor_msg_ctr++;
 			status = osMessageQueuePut(ContactorQueueHandle, &msg, 0, 0);
 			if(status != osOK)
 			{
 			 	contactorqueuefull++;
 		    }
 		 }
 	 }
}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef * hfdcan, uint32_t RxFifo0ITs)
{

		if ((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) == 0U) //&hfdcan1 is the CAN peripheral, FDCAN_RX/////-FIFO0 tells HALL to check FIFO 0, this function will return a number which will tell us how many messages are currently stored in RX FIFO
			return;
	    g_rx_cb0_hits++;

	    CANmsg msg = {0};
	    FDCAN_RxHeaderTypeDef rxHeader;

		if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &rxHeader, msg.data) != HAL_OK)//Try and read one CAN frame from the RX FIFO
		{
			return;
		}

		msg.extendedID				= rxHeader.Identifier; //full id no matter what
		msg.ID						= (uint16_t)(rxHeader.Identifier & 0x7FF); //mask CAN id with 0x7ff to keep the 11 bits
		msg.DLC						= rxHeader.DataLength;

//		if(ID == 0x210 && data[2] = 0x10)
//		{
//			TURNONGPIO23
//		}

		(void)osMessageQueuePut(canRxQueueHandle, &msg, 0, 0);
}

void HAL_FDCAN_RxFifo1Callback(FDCAN_HandleTypeDef * hfdcan, uint32_t RxFifo1ITs)
{

		if ((RxFifo1ITs & FDCAN_IT_RX_FIFO1_NEW_MESSAGE) == 0U) //&hfdcan1 is the CAN peripheral, FDCAN_RX/////-FIFO0 tells HALL to check FIFO 0, this function will return a number which will tell us how many messages are currently stored in RX FIFO
			return;
	    g_rx_cb1_hits++;

	    CANmsg msg = {0};
	    FDCAN_RxHeaderTypeDef rxHeader;

		if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO1, &rxHeader, msg.data) != HAL_OK)//Try and read one CAN frame from the RX FIFO
		{
			return;
		}

		msg.extendedID				= rxHeader.Identifier; //full id no matter what
		msg.ID						= (uint16_t)(rxHeader.Identifier & 0x7FF); //mask CAN id with 0x7ff to keep the 11 bits
		msg.DLC						= rxHeader.DataLength;

		(void)osMessageQueuePut(canRxQueueHandle, &msg, 0, 0);
}




