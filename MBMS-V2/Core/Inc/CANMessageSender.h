/*
 * CANMessageSender.h
 *
 *  Created on: Feb 4, 2026
 *      Author: m
 */

#ifndef INC_CANMESSAGESENDER_H_
#define INC_CANMESSAGESENDER_H_

// in Hz
#define HEARTBEAT_FREQ 1.0
#define CONTACTOR_COMMAND_FREQ 10.0
#define MBMS_STATUS_FREQ 10.0
#define DCDC_STACK_FREQ 10.0
#define MBMS_TRIP_FREQ 10.0
#define MBMS_STRIP_FREQ 10.0

#define NUM_CAN_MSG_TO_SEND 6


void CANMessageSenderTask(void * arg);
void CANMessageSender();

void send_MBMSHeartbeat();
void send_ContactorCommand();
void send_MBMSStatus();
void send_DCDCStack();
void send_MBMSTrips();
void send_MBMSSoftTrips();


void lastSentTime_init();

enum CANMessageToSend {
	HEARTBEAT = 0,
	CONTACTOR_COMMAND,
	MBMS_STATUS,
	DCDC_STACK,
	MBMS_TRIP,
	MBMS_STRIP // note: STRIP => Soft Trip
};




#endif /* INC_CANMESSAGESENDER_H_ */
