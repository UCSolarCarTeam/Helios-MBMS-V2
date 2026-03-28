#include "BatteryControlTask.h"
#include <stdint.h>
#include "cmsis_os.h"
#include "cmsis_os2.h"
#include "CAN.h"
//#include "StartupTask.h"
//#include "ShutoffTask.h"
//#include "ReadPowerGPIO.h"
//#include "CANMessageSenderTask.h"
#include "MBMS.h"
#include "main.h"
#include <string.h>
#include <stdbool.h>


extern osMessageQueueId_t ContactorQueueHandle; // Used GPT for this

uint32_t BCT_start_tick = 0;
uint32_t BCT_end_tick = 0;
uint32_t BCT_difference_tick = 0;
uint32_t BCT_difference_seconds = 0;
uint32_t BCT_Counter = 0;
uint32_t startup_Check_Counter = 0;
uint8_t carState = BOOT;

Contactor_Info contactorInfo[NUM_OF_CNTR] = {0};
MBMS_Status mbmsStatus;
BatteryInfo batteryInfo;
MBMS_Hard_Trips mbmsHardTrips;
MBMS_Soft_Trips mbmsSoftTrips;
Permissions mbmsPermissions;
Contactor_CMND_t contactorcmd;

uint32_t heartbeat_check_count = 0;
uint16_t previousHeartbeats[NUM_OF_CNTR] = {0};
heartbeatLastUpdatedTime[NUM_OF_CNTR] = {0};





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





void MBMSStatus_init(void)
{
    memset(&mbmsStatus, 0, sizeof(mbmsStatus));
    mbmsStatus.Abatt_enable = 1;
}




void perms_init()
{
    // Reset all system permissions to safe defaults.
    // This prevents the battery from charging or discharging
    // until the startup checks are complete.

    // TODO: set permission variables here
	mbmsPermissions.lv = 0;
	mbmsPermissions.motor = 0;
	mbmsPermissions.array = 0;
	mbmsPermissions.charge = 0;

}





void update_Counter(uint32_t *counter)
{
    // Make sure the pointer is valid
    if (counter != NULL)
    {
        // Increase the value of the counter by 1
        (*counter)++;
    }
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
	Control_Contactors();

	/* Updating BCT Counter */
	update_Counter(&BCT_Counter);

}




void Update_ContactorInfoStruct() {
	CANmsg contactorMsg;

    osStatus_t status = osMessageQueueGet(ContactorQueueHandle, &contactorMsg, NULL, 0); // Take from que and put into struct
    if (status != osOK) {
        return; // no new message
    }
    // defines are in CAN.h for mask and ID
    uint32_t extID = contactorMsg.extendedID;

    // if the msg is a contactor info msg
    if ((extID & CNTR_MSG_MASK) == CONTACTOR_ID){
    	uint8_t contactor_idx = extID - CONTACTOR_ID;

    	uint8_t *data = contactorMsg.data;

		uint8_t 	prechargerClosed   		= (data[0] >> 0) & 0x1;
		uint8_t 	prechargerClosing  		= (data[0] >> 1) & 0x1;
		uint8_t 	prechargerError    		= (data[0] >> 2) & 0x1;
		uint8_t 	contactorClosed    		= (data[0] >> 3) & 0x1;
		uint8_t 	contactorClosing   		= (data[0] >> 4) & 0x1;
		uint8_t 	contactorError     		= (data[0] >> 5) & 0x1;
		uint8_t 	contactorOpeningError 	= (data[0] >> 6) & 0x1;
		int16_t 	lineCurrent 			= ((data[0] & 0x80) >> 7) | (data[1] << 1) | ((data[2] & 0x07) << 9); // extract bits 7 to 18
		int16_t 	chargeCurrent 			= ((data[2] & 0xF8) >> 3) | ((data[3] & 0x7F) >> 6); // extract bits 19 to 30

		contactorInfo[contactor_idx].precharge_close = prechargerClosed;
		contactorInfo[contactor_idx].precharge_closing = prechargerClosing;
		contactorInfo[contactor_idx].precharge_error = prechargerError;
		contactorInfo[contactor_idx].contactor_close = contactorClosed;
		contactorInfo[contactor_idx].contactor_closing = contactorClosing;
		contactorInfo[contactor_idx].contactor_error = contactorError;
		contactorInfo[contactor_idx].contactor_opening_error = contactorOpeningError;
		contactorInfo[contactor_idx].line_current = lineCurrent;
		contactorInfo[contactor_idx].charge_current = chargeCurrent;


    }														// This is where we split the messages into heartbeat or board
    // if the msg is a contactor heartbeat msg. We split them again into which heartbeat CCP it is
    else {
    	uint8_t contactor_idx = extID - CONTACTOR_HEARTBEAT; // get index (which contactor it is)

    	uint16_t new_heartbeat = contactorMsg.data[0] + (contactorMsg.data[1] << 8);
    	contactorInfo[contactor_idx].heartbeat = new_heartbeat;
    }

    return;
}

//    void updateContactorInfo(uint8_t precharge_close,
//    							uint8_t precharge_closing,
//								uint8_t precharge_error,
//								uint8_t contactor_close,
//								uint8_t contactor_closing,
//								uint8_t contactor_error,
//								uint16_t line_current,
//								uint16_t charge_current,
//								uint8_t contactor_opening_error) {
//
//    }






/*-------------------------------------------*/
/* Startup checks */
void startupCheck() // change after this function is done: waitForFirstHeartbeats
{
    /* Run startup gate checks in order. If any fail, enter fault. */
    if (!waitForFirstHeartbeats())
    {
        enter_BPS_FAULT();   // preferred name from your header
        return;
    }

    if (!checkContactorsOpen() || !checkPrechargersOpen())
    {
    	enter_BPS_FAULT();
        return;
    }

    if (!startupBatteryCheck())
    {
    	enter_BPS_FAULT();
        return;
    }
}




//uint8_t waitForFirstHeartbeats() {
//
//
//	static uint8_t heartbeatFailCounter[NUM_OF_CNTR] = {0};
//	uint8_t dead = 0;
//
//
//		for(int i = 0; i < NUM_OF_CNTR; i++) {
//			heartbeat_check_count++;
//			if (heartbeatFailCounter[i] > MAX_HEARTBEAT_FAILS){
//
//				switch (i) {
//					case 0:
//						mbmsHardTrips.LV_no_heartbeat_trip = 1;
//						break;
//					case 1:
//						mbmsHardTrips.MT_no_heartbeat_trip = 1;
//						break;
//					case 2:
//						mbmsHardTrips.AR_no_heartbeat_trip = 1;
//						break;
//					case 3:
//						mbmsHardTrips.CHG_no_heartbeat_trip = 1;
//						break;
//				}
//
//				dead = 1;
//				return dead;
//
//			}
//			previousHeartbeats[i] = 0;
//
//			if (previousHeartbeats[i] >= 65535) { // check this logic lol
//				previousHeartbeats[i] = 0;
//			}
//
//			if(previousHeartbeats[i] >= contactorInfo[i].heartbeat){
//				if(((osKernelGetTickCount() - heartbeatLastUpdatedTime[i])) > CONTACTOR_HEARTBEAT_TIMEOUT) { // where contactor_heartbeat_timeout is how often a heartbeat is sent out/recieved
//					heartbeatFailCounter[i]++;
//
//				}
//			}
//			else {
//				heartbeatLastUpdatedTime[i] = osKernelGetTickCount();
//				heartbeatFailCounter[i] = 0;
//			}
//				previousHeartbeats[i] = (contactorInfo[i].heartbeat);
//
//			}
//
//
//		}
//		return dead;
//    return 0;
//
//
//
//}





uint8_t startupBatteryCheck()
{
    uint8_t pass = 1;

    if (batteryInfo.highCellVoltage > HARD_MAX_CELL_VOLTAGE)
    {
        mbmsHardTrips.High_volt_cell_trip = 1;
        pass = 0;
    }

    if (batteryInfo.lowCellVoltage < HARD_MIN_CELL_VOLTAGE)
    {
        mbmsHardTrips.Low_volt_cell_trip = 1;
        pass = 0;
    }

    if (batteryInfo.highTemp > HARD_MAX_TEMP)
    {
        mbmsHardTrips.High_temp_trip = 1;
        pass = 0;
    }

    if (batteryInfo.lowTemp < HARD_MIN_TEMP)
    {
        mbmsHardTrips.Low_temp_trip = 1;
        pass = 0;
    }

    startup_Check_Counter++;
    return pass;
}





uint8_t checkPrechargersOpen()
{
    for (int i = 0; i < NUM_OF_CNTR; i++)
    {
        /* If the precharger is reported closed, then it is NOT open. */
        if (contactorInfo[i].precharge_close == CLOSE_CONTACTOR)
        {
            return 0;
        }
    }
    return 1;
}





uint8_t checkContactorsOpen()
{
    for (int i = 0; i < NUM_OF_CNTR; i++)
    {
        if (contactorInfo[i].contactor_close == CLOSE_CONTACTOR)
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
    memset(&mbmsHardTrips, 0, sizeof(MBMS_Hard_Trips));

}





void clear_SoftTrips()
{
    /* Reset all soft battery trip flags */
    memset(&mbmsSoftTrips, 0, sizeof(MBMS_Soft_Trips));
}














// LATER TASKS //
void Update_DCDCStackStruct(void)
{
    /* Stub: define later when DCDC stack struct logic is ready */
}





void Update_BatteryInfoStruct(void)
{
    /* Stub: define later when battery info update logic is ready */
}





void SystemStateMachine(void)
{
    /* Stub: define later when state machine logic is ready */
}


void Control_Contactors()
{

	uint8_t contactorClosing = false;

	//check if any of the contactors are closed

	for (int i = 0; i < NUM_OF_CNTR; i++){
		if (contactorInfo[i].contactor_closing == CLOSING_CONTACTOR){
			contactorClosing = true;
			break;
		}
	}
	//if no contactors are in the process of closing and the battery is nor in fault tpe state (MPS,BPS)
	if (!contactorClosing && !mbmsPermissions.faulted){
		if((mbmsPermissions.lv) && (contactorInfo[LV].contactor_close != CLOSE_CONTACTOR) && (mbmsStatus.discharge_enable == 0)){
			contactorcmd.low_voltage = CLOSE_CONTACTOR;
		}
		else if ((mbmsPermissions.motor) && (contactorInfo[MOTOR].contactor_close != CLOSE_CONTACTOR ) && (mbmsStatus.discharge_enable == 0)){
				contactorcmd.motor = CLOSE_CONTACTOR;
		}
		else if ((mbmsPermissions.array) && (contactorInfo[ARRAY].contactor_close != CLOSE_CONTACTOR) && (mbmsStatus.charge_enable == 0)){
				contactorcmd.array = CLOSE_CONTACTOR;
		}
		else if ((mbmsPermissions.charge) && (contactorInfo[CHARGE].contactor_close != CLOSE_CONTACTOR) && (mbmsStatus.charge_enable == 0)){
				contactorcmd.charge = CLOSE_CONTACTOR;
		}
	}


	if (!mbmsPermissions.lv){
			contactorcmd.low_voltage = OPEN_CONTACTOR;
	}

	if (!mbmsPermissions.motor){
		contactorcmd.motor = OPEN_CONTACTOR;
	}

	if (!mbmsPermissions.array){
		contactorcmd.array = OPEN_CONTACTOR;
	}

	if (!mbmsPermissions.charge){
		contactorcmd.charge = OPEN_CONTACTOR;
	}
}







//void Update_TripStruct(){
//	static uint8_t BPS_Fault = 0;
//	osStatus_t acquire = osMutexAcquire()
//}






