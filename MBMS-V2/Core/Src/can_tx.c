#include "can_tx.h"
#include "cmsis_os2.h" //free rtos stuff
#include "main.h"

extern FDCAN_HandleTypeDef hfdcan1; //hfdcan defined in main.c

extern osMessageQueueId_t canTxQueueHandle; //defined in app_freertos.c

void CAN_Tx_Init(void)
{

if (HAL_FDCAN_Start(&hfdcan1) != HAL_OK)
	{
	Error_Handler();
	}
}

//Takes filled out CANmsg(ID,DLC,data...)
//Builds FDCAN_TXHeaderTypeDef from that info
//Call HAL_FDCAN_AddMessageToTxFifoQ() to send the frame
//Return HAL status to verify

HAL_StatusTypeDef CAN_Tx_Send(const CANmsg *msg) //CANmsg in CAN.h contains IDS,DLC data payload

{
	FDCAN_TxHeaderTypeDef txHeader; //tells hardware how to send

	//ID Setup
	txHeader.Identifier				= msg -> extendedID;
	txHeader.IdType					= FDCAN_EXTENDED_ID;
	txHeader.TxFrameType			= FDCAN_DATA_FRAME; //normal data frame with a payload not a remote req

	//DLC(storing encoded FDCAN_DLC_BYTES_xx in msg->DLC)
	txHeader.DataLength				= msg -> DLC;

	txHeader.ErrorStateIndicator	= FDCAN_ESI_ACTIVE; //says if transmitting node is error active or error passive
	txHeader.BitRateSwitch			= FDCAN_BRS_OFF; //No bit rate switching. Dont switch to higher data rate, just keep one speed the whole frame.
	txHeader.FDFormat				= FDCAN_CLASSIC_CAN; //Using classical CAN FRAMES. 	If want support from 12-64 bytes use FDCAN_FD_CAN instead
	txHeader.TxEventFifoControl		= FDCAN_NO_TX_EVENTS; //dont log anything. BUT can log stuff about transmitted frames. If want to use FDCAN_STORE_TX_EVENTS, which sores an entry each time a frame is sent
	txHeader.MessageMarker 			=0; // dont bother tagging frame. BUT a tiny tag(few bits)  can be attatched to frame so when a TX event is generated it can be matched to th ftame. Essentially a tiny ID you assign per frame

	//send frame to the TX FIFO
	return HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &txHeader, msg->data); //after call suceeds hardware will send frame out to the CAN BUS
	//&hfdcan1 pointer to global FDCAN handle in main.c. Tells HAL which CAN peripheral is being used(FDCAN1)
	//&txHeader points to FDCANTXHeaderTypeDef
	//msg -> data pointer to data in CANmsg struct
}

//Wait on canTxQueueHandle for CANmsg items and sends out one by one.

void CAN_Tx_Task(void *argument) //tasks run forever if theres nothing to return.
{
	(void)argument;
	CANmsg msg;


	for(;;)
	{
		//wait for message to appear
		//osMessageQueueGet take one item out of the queue
		//osStatus_t_status just tells us if it worked
		//after this messag eis returned msg should hold one complete CAN frame to send
		osStatus_t status = osMessageQueueGet
		(
		canTxQueueHandle, //Which queue to read from
		&msg, //pointer to CANmsg variable in this task
		NULL, //Not using any message priority feature
		osWaitForever //If queue empty block task unttil messgage is in queue
		);

		//status us what osMessageQueueGet returns
		if (status != osOK)
		{
			continue; //try again if queue fails. skips the rest of the loop body and goes back to the for(;;)

		}

		CAN_Tx_Send(&msg);
		// CAN_Tx_Send(&msg) transmits the CAN frame using the HAL
		//CAN_Tx_Send returns HAL_StatusTypeDef(HAL_OK ECT)


	}
}



