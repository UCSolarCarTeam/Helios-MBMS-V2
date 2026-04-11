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
//#include "BatteryControlTask.h"
//#include "StartupTask.h"

// Shared status and info structs

extern ContactorCommand contactorCommand;
extern PowerSelectionStatus powerSelectionStatus;
extern ContactorInfo contactorInfo[6];


/*
 * TO DO: extern the rest of the things u need :D
 *
 * Assume these instances already exist in the BCT src file (which we don't
 * have in this branch lol)
 *
 * Note: extern allows this file to use variables that you have declared in a
 * different file (in this case, BCT, which is why normally you would include the BCT
 * header file that I currently just have commented out as well)
 *
 */

extern BatteryInfo batteryInfo;
extern MBMSStatus mbmsStatus;
extern MBMSTrip mbmsTrip;
extern MBMSSoftBatteryLimitWarning mbmsSoftBatteryLimitWarning;

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


void CANMessageSender() {

	for (int i = 0; i < NUM_CAN_MSG_TO_SEND; i++) {
		if((osKernelGetTickCount() - (float)lastSentTime[i]) * FREERTOS_TICK_PERIOD >= 1/(messageFrequency[i])) {

			switch(i) {
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


void lastSentTime_init() {
	lastSentTime[HEARTBEAT] = osKernelGetTickCount();
	lastSentTime[CONTACTOR_COMMAND] = osKernelGetTickCount();
	lastSentTime[MBMS_STATUS] = osKernelGetTickCount();
	lastSentTime[DCDC_STACK] = osKernelGetTickCount();
	lastSentTime[MBMS_TRIP] = osKernelGetTickCount();
	lastSentTime[MBMS_STRIP] = osKernelGetTickCount();
}

// complete/finish the below functions please ! (Please use good names for things !)

void send_MBMSHeartbeat() {

	// creating an instance of a CAN msg
	CANmsg mbmsHeartbeatMsg;

	mbmsHeartbeatMsg.data[0] 		= 0x01;
	mbmsHeartbeatMsg.DLC 			= 1;
	mbmsHeartbeatMsg.extendedID 	= MBMS_HEARTBEAT_ID;
	mbmsHeartbeatMsg.ID 			= 0x0;
	osMessageQueuePut(canTxQueueHandle, &mbmsHeartbeatMsg, 0, osWaitForever);


}

void send_ContactorCommand() {

	CANmsg contactorCommandMsg;
	contactorCommandMsg.DLC			= 1;
	contactorCommandMsg.extendedID	= CONTACTOR_COMMAND_ID;
	contactorCommandMsg.ID			= 0x0;
	contactorCommandMsg.data[0]		= ((contactorCommand.common & 0x01) << COMMON) 	+
									  ((contactorCommand.motor & 0x01) << MOTOR) 	+
									  ((contactorCommand.array & 0x01) << ARRAY)   	+
									  ((contactorCommand.LV & 0x01) << LOWV)		+
									  ((contactorCommand.charge & 0x01) << CHARGE);
	osMessageQueuePut(canTxQueueHandle, &contactorCommandMsg, 0, osWaitForever);
}

void send_MBMSStatus() {
	CANmsg mbmsStatusMsg;
	uint16_t mbmsStatusData = ((mbmsStatus.auxilaryBattVoltage & 0x1f) << 0) + ((mbmsStatus.strobeBMSLight & 0x1) << 5)
			+ ((mbmsStatus.nChargeEnable & 0x1) << 6)   				     + ((mbmsStatus.nChargeSafety & 0x1) << 7)
			+ ((mbmsStatus.nDischargeEnable & 0x1) << 8)    				 + ((mbmsStatus.orionCANReceived & 0x1) << 9)
			+ ((mbmsStatus.dischargeShouldTrip & 0x1) << 10) 				 + ((mbmsStatus.chargeShouldTrip & 0x1) << 11)
			+ ((mbmsStatus.startupState & 0xf) << 12)						 + ((mbmsStatus.carState & 0x7) << 15);

	mbmsStatusMsg.data[0] = (mbmsStatusData & 0xff);
	mbmsStatusMsg.data[1] = (mbmsStatusData & 0xff00) >> 8;
	mbmsStatusMsg.data[2] = (mbmsStatusData & 0xff0000) >> 16;
	mbmsStatusMsg.DLC = 3; // 3 bytes as of may 21 (added carState..)
	mbmsStatusMsg.extendedID = MBMS_STATUS_ID;
	mbmsStatusMsg.ID = 0x0;
	osMessageQueuePut(canTxQueueHandle, &mbmsStatusMsg, 0, osWaitForever);

}



void send_DCDCStack() {
	CANmsg powSelectStatusMsg;
uint16_t data = ((powerSelectionStatus.nMainPowerSwitch & 0x1) << 0) + ((powerSelectionStatus.ExternalShutdown & 0x1) << 1)
		+ ((powerSelectionStatus.EN1 & 0x1) << 2) + ((powerSelectionStatus.nDCDC_Fault & 0x1) << 3)
		+ ((powerSelectionStatus.n3A_OC & 0x1) << 4) + ((powerSelectionStatus.nDCDC_On & 0x1) << 5)
		+ ((powerSelectionStatus.nCHG_Fault & 0x1) << 6) + ((powerSelectionStatus.nCHG_On & 0x1) << 7)
		+ ((powerSelectionStatus.nCHG_LV_En & 0x1) << 8) + ((powerSelectionStatus.ABATT_Disable & 0x1) << 9)
		+ ((powerSelectionStatus.Key & 0x1) << 10);
powSelectStatusMsg.data[0] = (data & 0xff);
powSelectStatusMsg.data[1] = (data & 0xff00) >> 8;
powSelectStatusMsg.DLC = 2;
powSelectStatusMsg.extendedID = POWER_SELECTION_STATUS_ID;
powSelectStatusMsg.ID = 0x0;
osMessageQueuePut(canTxQueueHandle, &powSelectStatusMsg, 0, osWaitForever);

}
void send_MBMSTrips() {
	CANmsg tripMsg;
	uint16_t tripData = ((mbmsSoftBatteryLimitWarning.highCellVoltageWarning & 0x1) << 0)   	+ ((mbmsSoftBatteryLimitWarning.lowCellVoltageWarning & 0x1) << 1)
			+ ((mbmsSoftBatteryLimitWarning.commonHighCurrentWarning & 0x1) << 2)   			+ ((mbmsSoftBatteryLimitWarning.motorHighCurrentWarning & 0x1) << 3)
			+ ((mbmsSoftBatteryLimitWarning.arrayHighCurrentWarning & 0x1) << 4)    			+ ((mbmsSoftBatteryLimitWarning.LVHighCurrentWarning & 0x1) << 5)
			+ ((mbmsSoftBatteryLimitWarning.chargeHighCurrentWarning & 0x1) << 6)   		    + ((mbmsSoftBatteryLimitWarning.highBatteryWarning & 0x1) << 7)
		    + ((mbmsSoftBatteryLimitWarning.highTemperatureWarning & 0x1 << 8))		     	+ ((mbmsSoftBatteryLimitWarning.lowTemperatureWarning & 0x1 << 9));

	tripMsg.data[0] = (tripData & 0xff);
	tripMsg.data[1] = (tripData & 0xff00) >> 8;
	tripMsg.DLC = 2; // 2 bytes
	tripMsg.extendedID = MBMS_SOFT_BATTERY_LIMIT_WARNING_ID;
	tripMsg.ID = 0x0;
	osMessageQueuePut(canTxQueueHandle, &tripMsg, 0, osWaitForever);


void send_MBMSSoftTrips() {
	CANmsg tripMsg;
	uint32_t tripData = ((mbmsTrip.highCellVoltageTrip & 0x1) << 0)   		  +	((mbmsTrip.lowCellVoltageTrip & 0x1) << 1)
			+ ((mbmsTrip.commonHighCurrentTrip & 0x1) << 2)   			  + ((mbmsTrip.motorHighCurrentTrip & 0x1) << 3)
			+ ((mbmsTrip.arrayHighCurrentTrip & 0x1) << 4)    			  + ((mbmsTrip.LVHighCurrentTrip & 0x1) << 5)
			+ ((mbmsTrip.chargeHighCurrentTrip & 0x1) << 6)   			  + ((mbmsTrip.protectionTrip & 0x1) << 7)
			+ ((mbmsTrip.orionMessageTimeoutTrip & 0x1) << 8) 			  + ((mbmsTrip.contactorDisconnectedUnexpectedlyTrip & 0x1) << 9)
			+ ((mbmsTrip.contactorConnectedUnexpectedlyTrip & 0x1) << 10) + ((mbmsTrip.highBatteryTrip & 0x1) << 11)
			+ ((mbmsTrip.commonHeartbeatDeadTrip & 0x1) << 12) 			  + ((mbmsTrip.motorHeartbeatDeadTrip & 0x1) << 13)
			+ ((mbmsTrip.arrayHeartbeatDeadTrip & 0x1) << 14)			  + ((mbmsTrip.LVHeartbeatDeadTrip & 0x1) << 15)
			+ ((mbmsTrip.chargeHeartbeatDeadTrip & 0x1) << 16)		      + ((mbmsTrip.MPSDisabledTrip & 0x1) << 17)
			+ ((mbmsTrip.ESDEnabledTrip & 0x1) << 18)					  + ((mbmsTrip.highTemperatureTrip & 0x1 << 19))
			+ ((mbmsTrip.lowTemperatureTrip & 0x1 << 20));

	tripMsg.data[0] = (tripData & 0xff);
	tripMsg.data[1] = (tripData & 0xff00) >> 8;
	tripMsg.data[2] = (tripData & 0xff0000) >> 16;
	tripMsg.DLC = 3; // 3 bytes
	tripMsg.extendedID = MBMS_TRIP_ID;
	tripMsg.ID = 0x0;
	osMessageQueuePut(canTxQueueHandle, &tripMsg, 0, osWaitForever);

}
}










