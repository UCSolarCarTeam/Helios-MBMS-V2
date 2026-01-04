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

void CAN_Rx_Task(void *argument)
{
	(void)argument; //shutting compiler up lol
	CANmsg msg;
	FDCAN_RxHeaderTypeDef rxHeader;

	for(;;) //RTOS tasks run forever. CAN_Rx_Task will sit in this forever loop always checking for new CAN frames.
	{
		if(HAL_FDCAN_GetRxFifoFillLevel(&hfdcan1,FDCAN_RX_FIFO0)>0U) //&hfdcan1 is the CAN peripheral, FDCAN_RX/////-FIFO0 tells HALL to check FIFO 0, this function will return a number which will tell us how many messages are currently stored in RX FIFO

		{
			if (HAL_FDCAN_GetRxMessage(&hfdcan1,FDCAN_RX_FIFO0,&rxHeader,msg.data) == HAL_OK)//Try and read one CAN frame from the RX FIFO
			{
				msg.extendedID				= rxHeader.Identifier; //full id no matter what
				msg.ID						= (uint16_t)(rxHeader.Identifier & 0x7FF); //mask CAN id with 0x7ff to keep the 11 bits

				msg.DLC						= rxHeader.DataLength;

			}
		}

		osDelay(1);

	}
}

