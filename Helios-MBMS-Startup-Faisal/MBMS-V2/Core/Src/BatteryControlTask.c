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

//fttytfjyfjmyfjm
//vhgmfdtydtyth


void enter_BOOT() {
    /* 1. Set system state to BOOT */
    carState = BOOT;

    /* 2. Clear all trip conditions */
    clear_Trips();
    clear_SoftTrips();

    /* 3. Reset permissions */
    perms_init();

    /* 4. Reset MBMS status */
    MBMSStatus_init();

    /* 5. Reset BCT timing variables */
    BCT_start_tick = 0;
    BCT_end_tick = 0;
    BCT_difference_tick = 0;
    BCT_difference_seconds = 0;

    /* 6. Reset internal BCT counter */
    BCT_Counter = 0;
}



void MBMSStatus_init() {
    /* Reset MBMS status to known-safe defaults */
    memset(&mbmsStatus, 0, sizeof(mbmsStatus));

    /* If you track timestamps/heartbeats, set them to "never seen" */
    mbmsStatus.lastHeartbeatTick = 0;

    /* If you track fault flags, they’re already cleared by memset.
       You can explicitly set any "expected" defaults here if needed. */

    /* Example: ensure we're not claiming readiness in BOOT */
    mbmsStatus.isReady = 0;

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




void UpdateContactorInfoStruct()
{
    osStatus_t status = osMessageQueueGet(contactorMessageQueueHandle, &contactorMsg, NULL, 0);
    if (status != osOK)
    {
        return; // no new message
    }  // good


    int idx = -1;

    switch (contactorMsg.extendedID) // LV, Motor,  // or contactorMsg.id depending on your msg type
    {
        case LV_board:  idx = 0; break;
        case Motor_board:  idx = 1; break;
        case Array_board:     idx = 2; break;
        case Charge_board: idx = 3; break;
        default: return; // not a contactor status message
    }



    //Task 2: Decode payload (bit/byte shifting)

    uint8_t *d = contactorMsg.data;


    uint8_t prechargerClosed   = (d[0] >> 0) & 0x1;
    uint8_t prechargerClosing  = (d[0] >> 1) & 0x1;
    uint8_t prechargerError    = (d[0] >> 2) & 0x1;
    uint8_t contactorClosed    = (d[0] >> 3) & 0x1;
    uint8_t contactorClosing   = (d[0] >> 4) & 0x1;
    uint8_t contactorError     = (d[0] >> 5) & 0x1;
    uint8_t BPSerror           = (d[0] >> 6) & 0x1;
//    int16_t lineCurrent  = (int16_t)( (int16_t)d[1] | ((int16_t)d[2] << 8) ); // might delete bit shifting if we change the data sheet to 16
    int16_t lineCurrent  = (int16_t)( (int16_t)d[1] | ((int16_t)d[2]));
//    int16_t chargeCurrent= (int16_t)( (int16_t)d[3] | ((int16_t)d[4] << 8) ); // might delete bit shifting if we change the data sheet to 16
    int16_t chargeCurrent= (int16_t)( (int16_t)d[3] | ((int16_t)d[4]));


    updateContactorInfo((uint8_t)idx,
                        prechargerClosed, prechargerClosing, prechargerError,
                        contactorClosed,  contactorClosing,  contactorError,
                        lineCurrent, chargeCurrent, BPSerror);


    int heartbeat = 3;
    // update the list
}










/*-------------------------------------------*/
/* Startup checks */

// FASIAL PLEASE WORK ON THESE ONES !!!! :D

void startupCheck()
{
    /* Run startup gate checks in order. If any fail, enter fault. */
    if (waitForFirstHeartbeats())
    {
        initiateBPSFault();   // preferred name from your header
        return;
    }

    if (!checkContactorsOpen() || !checkPrechargersOpen())
    {
        initiateBPSFault();
        return;
    }

    if (!startupBatteryCheck())
    {
        initiateBPSFault();
        return;
    }
}




uint8_t waitForFirstHeartbeats() {
	osStatus status = osMessageQueueGet(contactorMessageQueueHandle, &contactorMsg, NULL, 0);
	if (status != osOK)
	    {
	        // No new message; just indicate "not dead yet" or keep waiting

	        return 0;
	    }
	int message = "10101000000000000000000000000000000";
	for (int i = 0; i < 3; i++){
		board_list[i] // getting the contactor struct

				   struct -> chargingState = dskfo

	}


}




uint8_t startupBatteryCheck()
{
    uint8_t pass = 1;

    if (batteryInfo.highCellVoltage > HARD_MAX_CELL_VOLTAGE)
    {
        mbmsTrip.highCellVoltageTrip = 1;
        pass = 0;
    }

    if (batteryInfo.lowCellVoltage < HARD_MIN_CELL_VOLTAGE)
    {
        mbmsTrip.lowCellVoltageTrip = 1;
        pass = 0;
    }

    if (batteryInfo.highTemp > HARD_MAX_TEMP)
    {
        mbmsTrip.highTemperatureTrip = 1;
        pass = 0;
    }

    if (batteryInfo.lowTemp < HARD_MIN_TEMP)
    {
        mbmsTrip.lowTemperatureTrip = 1;
        pass = 0;
    }

    startup_Check_Counter++;
    return pass;
}





uint8_t checkPrechargersOpen()
{
    for (int i = 1; i < 5; i++)
    {
        /* If the precharger is reported closed, then it is NOT open. */
        if (contactorInfo[i].prechargerClosed == CLOSE_CONTACTOR)
        {
            return 0;
        }
    }

    return 1;
}

uint8_t checkContactorsOpen()
{

    for (int i = 0; i < 5; i++)
    {
        if (contactorInfo[i].contactorClosed == CLOSE_CONTACTOR)
        {
            return 0;
        }
    }

    return 1;
}

/*-------------------------------------------*/

// Had to use AI for these bottom functions:
void clear_Trips()
{
    /* Reset all hard trip flags to 0 */
    memset(&mbmsTrip, 0, sizeof(mbmsTrip));

}


void clear_SoftTrips()
{
    /* Reset all soft warning flags */
    memset(&mbmsSoftBatteryLimitWarning, 0, sizeof(mbmsSoftBatteryLimitWarning));

    /* Reset all soft battery trip flags */
    memset(&softBatteryTrip, 0, sizeof(softBatteryTrip));
}

//hallo










