/*
 * CANMessageSender.c
 *
 *  Created on: Feb 4, 2026
 *      Author: m
 */

#include "CANMessageSender.h"
#include "MBMS.h"
#include "CAN.h"
#include "app_freertos.h"
#include "BatteryControlTask.h"
//#include "StartupTask.h"


extern Contactor_Command contactorCommand;
extern Contactor_Info contactorInfo[NUM_OF_CNTR];
extern Battery_Info batteryInfo;
extern MBMS_Status mbmsStatus;
extern MBMS_Hard_Trips mbmsHardTrips;
extern MBMS_Soft_Trips mbmsSoftTrips;
extern DCDC_Stack dcdc_stack;

// in ticks !
uint32_t lastSentTime[NUM_CAN_MSG_TO_SEND] = {0};

// frequencies each message should be sent at
float messageFrequency[NUM_CAN_MSG_TO_SEND] = { HEARTBEAT_FREQ, CONTACTOR_COMMAND_FREQ,
								MBMS_STATUS_FREQ, DCDC_STACK_FREQ,
								MBMS_TRIP_FREQ, MBMS_STRIP_FREQ};


void CANMessageSenderTask(void* arg)

{
	uint32_t taskTickLastStart = osKernelGetTickCount();
	lastSentTime_init();
    while(1)
    {
    	CANMessageSender();
		taskTickLastStart += 10;
		osDelayUntil(taskTickLastStart);
    }
}


void CANMessageSender()
{
	{
	for (int i = 0; i < NUM_CAN_MSG_TO_SEND; i++) {
		if((osKernelGetTickCount() - (float)lastSentTime[i]) * FREERTOS_TICK_PERIOD >= 1/(messageFrequency[i]))
		{

			switch(i)
			{
				case HEARTBEAT:
					send_MBMSHeartbeat();
					break;

				case CONTACTOR_COMMAND:
					send_ContactorCommand();
					break;

				case MBMS_STATUS:
					send_MBMSStatus();
					break;

				case DCDC_STACK:
					send_DCDCStack();
					break;

				case MBMS_TRIP:
					send_MBMSTrips();
					break;

				case MBMS_STRIP:
					send_MBMSSoftTrips();
					break;
			}
			lastSentTime[i] = osKernelGetTickCount();
			osDelay(10);

			}
		}
	}
}


void lastSentTime_init() {
	lastSentTime[HEARTBEAT] = osKernelGetTickCount();
	lastSentTime[CONTACTOR_COMMAND] = osKernelGetTickCount();
	lastSentTime[MBMS_STATUS] = osKernelGetTickCount();
	lastSentTime[DCDC_STACK] = osKernelGetTickCount();
	lastSentTime[MBMS_TRIP] = osKernelGetTickCount();
	lastSentTime[MBMS_STRIP] = osKernelGetTickCount();
}

// complete/finish the below functions please ! (Please use good names for things !)


// checked good
void send_MBMSHeartbeat() {

	// creating an instance of a CAN msg
	CANmsg mbmsHeartbeatMsg;

	mbmsHeartbeatMsg.data[0] 		= 0x01; // just sending ones
	mbmsHeartbeatMsg.DLC 			= 1;
	mbmsHeartbeatMsg.extendedID 	= MBMS_HEARTBEAT_ID;
	mbmsHeartbeatMsg.ID 			= 0x0;
	osMessageQueuePut(canTxQueueHandle, &mbmsHeartbeatMsg, 0, osWaitForever);


}

// checked good
void send_ContactorCommand()
{
	CANmsg contactorCommandMsg;
	contactorCommandMsg.DLC			= 1;
	contactorCommandMsg.extendedID	= CONTACTOR_COMMAND_ID;
	contactorCommandMsg.ID			= 0x0;
	osStatus_t acquire_1 = osMutexAcquire(ContactorCommandMutexHandle, READING_MUTEX_TIMEOUT);
	if(acquire_1 == osOK)
	{
		contactorCommandMsg.data[0]		= ((contactorCommand.LV & 0x01)     << LV)      +
										  ((contactorCommand.motor1 & 0x01) << MOTOR1) 	+
#if motor2def
										  ((contactorCommand.motor2 & 0x01) << MOTOR2) 	+
#endif
										  ((contactorCommand.array & 0x01)  << ARRAY)   +
										  ((contactorCommand.charge & 0x01) << CHARGE);
		osMutexRelease(ContactorCommandMutexHandle);
	}
	osMessageQueuePut(canTxQueueHandle, &contactorCommandMsg, 0, osWaitForever);
}

// checked good
void send_MBMSStatus()
{
	CANmsg mbmsStatusMsg;
	uint16_t mbmsStatusData;
	osStatus_t acquire_2 = osMutexAcquire(MBMSStatusMutexHandle, READING_MUTEX_TIMEOUT);
	if(acquire_2 == osOK)
	{
		mbmsStatusData = ((mbmsStatus.BPS_Fault & 0x01) << 0) + ((mbmsStatus.charge_safety & 0x1) << 1)
				+ ((mbmsStatus.discharge_enable & 0x1) << 2)   		   + ((mbmsStatus.charge_enable & 0x1) << 3)
				+ ((mbmsStatus.OBMS_CAN_RR & 0x1) << 4)    			   + ((mbmsStatus.MPS & 0x1) << 5)
				+ ((mbmsStatus.ESD & 0x1) << 6) 				       + ((mbmsStatus.Abatt_EN & 0x1) << 7)
				+ ((mbmsStatus.EVCC_12V_Sw & 0x1) << 8)			       + ((mbmsStatus.Startup_state & 0xf) << 9)
				+ ((mbmsStatus.System_state & 0x7) << 13);
		osMutexRelease(MBMSStatusMutexHandle);
	}
	mbmsStatusMsg.data[0] = (mbmsStatusData & 0xff);
	mbmsStatusMsg.data[1] = (mbmsStatusData & 0xff00) >> 8;

	mbmsStatusMsg.DLC = 2;
	mbmsStatusMsg.extendedID = MBMS_STATUS_ID;
	mbmsStatusMsg.ID = 0x0;
	osMessageQueuePut(canTxQueueHandle, &mbmsStatusMsg, 0, osWaitForever);
}


// checked good
void send_DCDCStack()
{
	CANmsg DCDCStackMsg;
	uint16_t data;
	osStatus_t acquire_3 = osMutexAcquire(DCDCStackMutexHandle, READING_MUTEX_TIMEOUT);
	if(acquire_3 == osOK)
	{
		data = ((dcdc_stack.DCDC1_en & 0x1) << 0) + ((dcdc_stack._14V_Charge_EN & 0x1) << 1)
		+ ((dcdc_stack.nDCDC_Fault & 0x1) << 2) + ((dcdc_stack._12V_Critical_Fault & 0x1) << 3)
		+ ((dcdc_stack._14V_Charger_Fault & 0x1) << 4) + ((dcdc_stack._12V_Critical_UC & 0x1) << 5);
		osMutexRelease(DCDCStackMutexHandle);
	}
	DCDCStackMsg.data[0] = (data & 0xff);
	DCDCStackMsg.DLC = 1;
	DCDCStackMsg.extendedID = DCDC_STACK_ID;
	DCDCStackMsg.ID = 0x0;
	osMessageQueuePut(canTxQueueHandle, &DCDCStackMsg, 0, osWaitForever);
}

// checked good
void send_MBMSSoftTrips()
{
	CANmsg tripMsg;
	uint16_t tripData;
	osStatus_t acquire_4 = osMutexAcquire(MBMSSoftTripMutexHandle, READING_MUTEX_TIMEOUT);
	if(acquire_4 == osOK)
	{
		tripData = ((mbmsSoftTrips.High_volt_cell_Strip & 0x1) << 0)   	+	((mbmsSoftTrips.Low_volt_cell_Strip & 0x1) << 1)
				 + ((mbmsSoftTrips.CMN_high_cur_Strip & 0x1) << 2)       +	((mbmsSoftTrips.LV_high_cur_Strip & 0x1) << 3)

				 + ((mbmsSoftTrips.MT1_high_cur_Strip & 0x1) << 4)
				 + ((mbmsSoftTrips.AR_high_cur_Strip & 0x1) << 5)        +	((mbmsSoftTrips.CHG_high_cur_Strip & 0x1) << 6)
				 + ((mbmsSoftTrips.High_temp_Strip & 0x1) << 7)          +	((mbmsSoftTrips.Low_temp_Strip & 0x1 << 8));
		osMutexRelease(MBMSSoftTripMutexHandle);
	}
	tripMsg.data[0] = (tripData & 0xff);
	tripMsg.data[1] = (tripData & 0xff00) >> 8;
	tripMsg.DLC = 2; // 2 bytes
	tripMsg.extendedID = MBMS_SOFT_TRIP_ID;
	tripMsg.ID = 0x0;
	osMessageQueuePut(canTxQueueHandle, &tripMsg, 0, osWaitForever);
}


//checked good
void send_MBMSTrips()
{
	CANmsg tripMsg;
	uint16_t tripData;
	osStatus_t acquire_5 = osMutexAcquire(MBMSTripMutexHandle, READING_MUTEX_TIMEOUT);
	if(acquire_5 == osOK)
	{
		tripData = ((mbmsHardTrips.High_volt_cell_trip & 0x1) << 0)     +	((mbmsHardTrips.Low_volt_cell_trip & 0x1) << 1)
				+ ((mbmsHardTrips.CMN_high_cur_trip & 0x1) << 2)   	    + ((mbmsHardTrips.LV_high_cur_trip & 0x1) << 3)
				+ ((mbmsHardTrips.MT1_high_cur_trip & 0x1) << 4)
				+ ((mbmsHardTrips.AR_high_cur_trip & 0x1) << 5)			+ ((mbmsHardTrips.CHG_high_cur_trip & 0x1) << 6)
				+ ((mbmsHardTrips.Reverse_cur_trip & 0x1) << 7)			+ ((mbmsHardTrips.OBMS_msg_timeout_trip & 0x1) << 8)
				+ ((mbmsHardTrips.CNTR_disconnect_trip & 0x1) << 9)		+ ((mbmsHardTrips.CNTR_connect_trip & 0x1) << 10)
				+ ((mbmsHardTrips.CMN_no_heartbeat_trip & 0x1) << 11)	+ ((mbmsHardTrips.LV_no_heartbeat_trip & 0x1) << 12)
				+ ((mbmsHardTrips.MT1_no_heartbeat_trip & 0x1) << 13)
				+ ((mbmsHardTrips.AR_no_heartbeat_trip & 0x1) << 14)	+ ((mbmsHardTrips.CHG_no_heartbeat_trip & 0x1) << 15)
				+ ((mbmsHardTrips.ESD_trip & 0x1) << 16)		        + ((mbmsHardTrips.High_temp_trip & 0x1) << 17)
				+ ((mbmsHardTrips.Low_temp_trip & 0x1) << 18);
		osMutexRelease(MBMSTripMutexHandle);
	}
	tripMsg.data[0] = (tripData & 0xff);
	tripMsg.data[1] = (tripData & 0xff00) >> 8;
	tripMsg.data[2] = (tripData & 0xff0000) >> 16;
	tripMsg.DLC = 3; // 3 bytes
	tripMsg.extendedID = MBMS_TRIP_ID;
	tripMsg.ID = 0x0;

	osMessageQueuePut(canTxQueueHandle, &tripMsg, 0, osWaitForever);
}











