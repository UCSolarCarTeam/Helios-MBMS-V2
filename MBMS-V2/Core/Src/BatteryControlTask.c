#include "BatteryControlTask.h"
#include <stdint.h>
#include "cmsis_os.h"
#include "CAN.h"
//#include "StartupTask.h"
//#include "ShutoffTask.h"
//#include "ReadPowerGPIO.h"
//#include "CANMessageSenderTask.h"
#include "MBMS.h"
#include "main.h"

#include <string.h>

#define HARD_MAX_CELL_VOLTAGE 4.20f //4.20V
#define HARD_MIN_CELL_VOLTAGE 2.50f // 2.50V
#define HARD_MAX_TEMP 60
#define HARD_MIN_TEMP 0

extern osMessageQueueId_t ContactorQueueHandle; // Used GPT for this

// test !!!

uint32_t BCT_start_tick = 0;
uint32_t BCT_end_tick = 0;
uint32_t BCT_difference_tick = 0;
uint32_t BCT_difference_seconds = 0;
uint32_t BCT_Counter = 0;


Contactor_Info contactorInfo[NUM_OF_CNTR] = {0};
MBMS_Status mbmsStatus;
BatteryInfo batteryInfo;
MBMS_Hard_Trips mbmsHardTrips;
MBMS_Soft_Trips mbmsSoftTrips;

uint32_t startup_Check_Counter = 0;

uint8_t carState = BOOT;









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
	UpdateContactorInfoStruct();
	Update_DCDCStackStruct();
	Update_BatteryInfoStruct();


	/* Tracking states */
	SystemStateMachine();

	/* Opening/closing contactors */
	UpdateContactors();

	/* Updating BCT Counter */
	UpdateCounter(&BCT_Counter);

}



// Instead of a switch case. make a instance of a contatcor info struct.
// Contatctor_Info LV_info;
// Contatctor_Info Motor_info;
// Contatctor_Info Array_info;
// LV_info.charge_current = 6;
// Motor_info.contactor_close = 1;

//Remember structs are a data type with things inside of it so just make instances of it

// Or use an array of contactor info
void UpdateContactorInfoStruct()
{

	CANmsg contactorMsg;

    osStatus_t status = osMessageQueueGet(ContactorQueueHandle, &contactorMsg, NULL, 0);
    if (status != osOK)
    {
        return; // no new message
    }  // good


    int idx = -1;

    switch (contactorMsg.extendedID) // LV, Motor,  // or contactorMsg.id depending on your msg type
    {
        case LV:
        	idx = 0; break;
        case MOTOR:
        	idx = 1; break;
        case ARRAY:
        	idx = 2; break;
        case CHARGE:
        	idx = 3; break;

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
}










/*-------------------------------------------*/
/* Startup checks */
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

//	CANmsg contactorMsg;
//
//	osStatus status = osMessageQueueGet(ContactorQueueHandle, &contactorMsg, NULL, 0);
//	if (status != osOK)
//	    {
//	        // No new message; just indicate "not dead yet" or keep waiting
//
//	        return 0;
//	    }
//	int message = "10101000000000000000000000000000000";
//
//	for (int i = 0; i < 3; i++){
////		board_list[i] // getting the contactor struct
//	}
//}
    return 0;
}



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
    for (int i = 1; i < NUM_OF_CNTR; i++)
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

void UpdateContactors(void)
{
    /* Stub: define later when contactor actuation logic is ready */
}









