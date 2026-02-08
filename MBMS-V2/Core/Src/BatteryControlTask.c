/*
 * BatteryControlTask.c
 *
 *  Created on: Jan 17, 2026
 *      Author: m
 */


#include "BatteryControlTask.h"
#include <stdint.h>
#include "cmsis_os.h"
//#include "CANdefines.h"
//#include "StartupTask.h"
//#include "ShutoffTask.h"
//#include "ReadPowerGPIO.h"
//#include "CANMessageSenderTask.h"
//#include "MBMS.h"
//#include "main.h"


uint32_t BCT_start_tick = 0;
uint32_t BCT_end_tick = 0;
uint32_t BCT_difference_tick = 0;

uint32_t BCT_difference_seconds = 0;

uint32_t BCT_Counter = 0;

void enter_BOOT() {
	// TO DO
}

void MBMSStatus_init() {
// TO DO

}

void perms_init() {
	// TO DO

}

void UpdateCounter(uint32_t * counter) {

}




void BatteryControlTask(void* arg)
{
	uint32_t taskTickLastStart = osKernelGetTickCount();

    while(1)
    {
    	BCT_start_tick = osKernelGetTickCount();
    	BatteryControl();
    	BCT_end_tick = osKernelGetTickCount();
    	BCT_difference_tick = BCT_end_tick - BCT_start_tick;
    	BCT_difference_seconds = taskTickLastStart;

		taskTickLastStart += 10;
		osDelayUntil(taskTickLastStart);
    }
}

void BatteryControl() {

	/* Updating structs */
	Update_ContactorInfoStruct();
	Update_DCDCStackStruct();
	Update_BatteryInfoStruct();


	/* Tracking states */
	SystemStateMachine();

	/* Opening/closing contactors */
	UpdateContactors();

	/* Updating BCT Counter */
	UpdateCounter(&BCT_Counter);

}


void UpdateContactorInfoStruct() {

}

/*-------------------------------------------*/
/* Startup checks */

// FASIAL PLEASE WORK ON THESE ONES !!!! :D

void startupCheck() {

}
uint8_t waitForFirstHeartbeats() {

	uint8_t pass = 0;

	return pass;

}

uint8_t startupBatteryCheck() {
	uint8_t pass = 0;

	return pass;
}

uint8_t checkPrechargersOpen() {
	uint8_t pass = 0;

	return pass;
}

uint8_t checkContactorsOpen() {
	uint8_t pass = 0;

	return pass;
}

/*-------------------------------------------*/


void clear_Trips() {

}
void clear_SoftTrips() {

}












