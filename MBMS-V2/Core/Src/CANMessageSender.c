/*
 * CANMessageSender.c
 *
 *  Created on: Feb 4, 2026
 *      Author: m
 */

#include "CANMessageSender.h"
#include "MBMS.h"
#include "CAN.h"
//#include "BatteryControlTask.h"
//#include "StartupTask.h"

// Shared status and info structs

extern ContactorCommand contactorCommand;

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
	CANmsg MBMSHeartbeatMsg;

	// DO THE REST BELOW:

	// fill out data, DLC, extendedID, ID (set to 0x0)


	// add msg to CAN TX msg queue

}

void send_ContactorCommand() {

}

void send_MBMSStatus() {

}

void send_DCDCStack() {

}

void send_MBMSTrips() {

}

void send_MBMSSoftTrips() {

}










