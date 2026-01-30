/*
 * can_rx.c
 *
 *  Created on: Jan 4, 2026
 *      Author: helay
 */


#include "can_rx.h"
#include "main.h"
#include "cmsis_os2.h"

extern FDCAN_HandleTypeDef hfdcan1; //defined in main.c
extern osMessageQueueId_t canRxQueueHandle;
extern osMessageQueueId_t BatteryQueueHandle;
extern osMessageQueueId_t ContactorQueueHandle;

volatile uint32_t g_rx_cb_hits = 0;

static void CAN_Rx(void);

void CAN_Rx_Task(void *argument)
{
	(void)argument;
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

 if (msg.extendedID == 0x100 &&
           msg.DLC == FDCAN_DLC_BYTES_8 &&
           msg.data[0] == 0x11 &&
           msg.data[1] == 0x22 &&
           msg.data[2] == 0x33 &&
           msg.data[3] == 0x44 &&
           msg.data[4] == 0x55 &&
           msg.data[5] == 0x66 &&
           msg.data[6] == 0x77 &&
           msg.data[7] == 0x88)
 {
	 //HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
 }
 //else if (status == osOK){
	 //uint32_t eID = msg.extendedID;

	// if(eID == PACK_INFO_ID || eID == TEMPINFOID || eID == MAXMINVOLTAGESID){

		// status = osMessageQueuePut(batteryControlMessageQueueHandle, &msg, 0, osWaitForever);
		 	// if(status != osOK){
//}
 }
	// DEQUEUE
	// CHECK WHAT MSG IT IS (EID)
	// SPLIT INTO 2 DIFF QUEUES (contactors, battery/orion)


    if (status == osOK) return;
    //temporary until starting message task
    osMessageQueueId_t dst = (msg.extendedID & 1U) ? contactorQueueHandle : batteryQueueHandle;
    (void)osMessageQueuePut(dst, &msg, 0, 0);

}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef * hfdcan, uint32_t RxFifo0ITs)
{

		if ((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) == 0U) //&hfdcan1 is the CAN peripheral, FDCAN_RX/////-FIFO0 tells HALL to check FIFO 0, this function will return a number which will tell us how many messages are currently stored in RX FIFO
			return;
	    g_rx_cb_hits++;

	    CANmsg msg = {0};
	    FDCAN_RxHeaderTypeDef rxHeader;

		if (HAL_FDCAN_GetRxMessage(&hfdcan, FDCAN_RX_FIFO0, &rxHeader, msg.data) != HAL_OK)//Try and read one CAN frame from the RX FIFO

			return;

			msg.extendedID				= rxHeader.Identifier; //full id no matter what
			msg.ID						= (uint16_t)(rxHeader.Identifier & 0x7FF); //mask CAN id with 0x7ff to keep the 11 bits
			msg.DLC						= rxHeader.DataLength;

		(void)osMessageQueuePut(canRxQueueHandle, &msg, 0, 0);
}





