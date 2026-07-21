#include "BatteryControlTask.h"
#include <stdint.h>
#include "cmsis_os.h"
#include "cmsis_os2.h"
#include "CAN.h"
#include "StartupTask.h"
//#include "ShutoffTask.h"
#include "ReadGPIO.h"
//#include "CANMessageSenderTask.h"
#include "MBMS.h"
#include "main.h"
#include <string.h>
#include <stdbool.h>
#include <app_freertos.h>

#include <stdlib.h>

#define test_with_CCPs 1 // comment out the trips for heartbeats..

uint32_t BCT_start_tick = 0;
uint32_t BCT_end_tick = 0;
uint32_t BCT_difference_tick = 0;
uint32_t BCT_difference_seconds = 0;
uint32_t BCT_Counter = 0;
uint32_t startup_Check_Counter = 0;
uint8_t carState = BOOT;

// creates a struct that stores information about one part of the system
Contactor_Info contactorInfo[NUM_OF_CNTR] = {0};
MBMS_Status mbmsStatus;
Battery_Info batteryInfo;
MBMS_Hard_Trips mbmsHardTrips;
MBMS_Soft_Trips mbmsSoftTrips;
Permissions mbmsPermissions;
Contactor_Command contactorCommand;
DCDC_Stack dcdc_stack;

uint32_t heartbeat_enter_SOFT_TRIP_count = 0;
uint16_t previousHeartbeats[NUM_OF_CNTR] = {0};
uint32_t heartbeatLastUpdatedTime[NUM_OF_CNTR] = {0}; // enter_SOFT_TRIP datatype..

uint32_t pack_info_counter = 0;
uint32_t temp_info_counter = 0;
uint32_t cell_voltages_counter = 0;

static uint32_t missingOBMS_MsgCounter = 0;

uint8_t main_cmn_cntr_trip = 0;

void MBMSStatus_init(void)
{
	osStatus_t MBMSStatus_a1 = osMutexAcquire(MBMSStatusMutexHandle, UPDATING_MUTEX_TIMEOUT );
	if(MBMSStatus_a1 == osOK)
	{
		memset(&mbmsStatus, 0, sizeof(mbmsStatus));
		mbmsStatus.Abatt_EN = 1;
		osMutexRelease(MBMSStatusMutexHandle);
	}
}



// mutexes look good here
void perms_init()
{
    // Reset all system permissions to safe defaults.
    // This prevents the battery from charging or discharging
    // until the startup enter_SOFT_TRIPs are complete.
	osStatus_t Permissions_a1 = osMutexAcquire(PermissionsMutexHandle, UPDATING_MUTEX_TIMEOUT );
	if(Permissions_a1 == osOK)
	{
		// TODO: set permission variables here
		mbmsPermissions.lv = 0;
		mbmsPermissions.motor1 = 0;
		mbmsPermissions.motor2 = 0;
		mbmsPermissions.array = 0;
		mbmsPermissions.charge = 0;
		osMutexRelease(PermissionsMutexHandle);
	}

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

		taskTickLastStart += 10; // BCT should take 10 ticks to run 1 cycle !!!!
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



/* ------ System State Switching Functions ------ */

void enter_BOOT()
{
	HAL_GPIO_WritePin(DB_R_GPIO_Port, DB_R_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(DB_B_GPIO_Port, DB_B_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(DB_G_GPIO_Port, DB_G_Pin, GPIO_PIN_SET);

    /* 1. Set system state to BOOT */
	osStatus_t mbmsStatus_a1 = osMutexAcquire(MBMSStatusMutexHandle, UPDATING_MUTEX_TIMEOUT );
	if(mbmsStatus_a1 == osOK) {
		mbmsStatus.System_state = BOOT;
		osMutexRelease(MBMSStatusMutexHandle);
	}

    carState = BOOT;
    mbmsStatus.System_state = BOOT;
    // erm should prob put mutex but i kinda lazy rn idk

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

    /* 6. Reset Counters */
    BCT_Counter = 0;
    missingOBMS_MsgCounter = 0;
    startup_Check_Counter = 0;
	pack_info_counter = 0;
	temp_info_counter = 0;
	cell_voltages_counter = 0;

	/* 7. Reset Heartbeats */
	for(int i = 0; i < NUM_OF_CNTR; i++)
	{
		previousHeartbeats[i] = 0;
		heartbeatLastUpdatedTime[i] = 0;
		osStatus_t ContactorInfo_a1 = osMutexAcquire(ContactorInfoMutexHandle, UPDATING_MUTEX_TIMEOUT );
		if(ContactorInfo_a1 == osOK)
		{
			contactorInfo[i].heartbeat = 0;
			osMutexRelease(ContactorInfoMutexHandle);
		}
	}

	// Not too sure if we need this here, cuz have in sys state.... cuz just runs once here
//	/* 8. Make sure contactors open */
//	if (!checkContactorsOpen() || !checkPrechargersOpen())
//	{
//		enter_BPS_FAULT();
//		return;
//	}

}


// NOTE: actually check stuff that runs "once" bc if it skips the faulted step that kinda bad icl...
// e.g. if it only calls this function once kinda thing.. idk ... etc.
// prob relevant for all enter funcs and idk what else
// BUT ALSOOOOOOO like should prob be ok if literally nothing else is updating perms (minus startup)
// just check i guess !



void enter_MPS_DISCONNECTED()
{
	//mbmsHardTrips
	carState = MPS_DISCONNECTED;
	osStatus_t mbmsStatus_a1 = osMutexAcquire(MBMSStatusMutexHandle, UPDATING_MUTEX_TIMEOUT );
	if(mbmsStatus_a1 == osOK) {
		mbmsStatus.System_state = MPS_DISCONNECTED;
		mbmsStatus.MPS = read_MPS();
		osMutexRelease(MBMSStatusMutexHandle);
	}

	osStatus_t Permissions_a2 = osMutexAcquire(PermissionsMutexHandle, UPDATING_MUTEX_TIMEOUT );
	if(Permissions_a2 == osOK)
	{
		mbmsPermissions.faulted = 1; //contactors not allowed to close
		osMutexRelease(PermissionsMutexHandle);
	}
	osEventFlagsSet(shutoffFlagHandle, (MPS_FLAG | SHUTOFF_FLAG)); //idk if I understnad this
}





void enter_BPS_FAULT()
{
	HAL_GPIO_WritePin(DB_R_GPIO_Port, DB_R_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(DB_B_GPIO_Port, DB_B_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(DB_G_GPIO_Port, DB_G_Pin, GPIO_PIN_RESET);

	HAL_GPIO_WritePin(BPS_Fault_GPIO_Port, BPS_Fault_Pin, BPS_FAULT_ACTIVE); // strobe enabling essentially
	osStatus_t Permissions_a3 = osMutexAcquire(PermissionsMutexHandle, UPDATING_MUTEX_TIMEOUT );
	if(Permissions_a3 == osOK)
	{
		mbmsPermissions.faulted = 1;
		osMutexRelease(PermissionsMutexHandle);
	}

	osStatus_t mbmsStatus_a1 = osMutexAcquire(MBMSStatusMutexHandle, UPDATING_MUTEX_TIMEOUT);
	if(mbmsStatus_a1 == osOK) {
		mbmsStatus.System_state = BPS_FAULT;
		mbmsStatus.BPS_Fault = 1;
		osMutexRelease(MBMSStatusMutexHandle);
	}
	carState = BPS_FAULT;

	osEventFlagsSet(shutoffFlagHandle, (HARD_BAT_LIMIT_FLAG | SHUTOFF_FLAG));
}





void enter_SOFT_TRIP()
{
	HAL_GPIO_WritePin(DB_R_GPIO_Port, DB_R_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(DB_B_GPIO_Port, DB_B_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(DB_G_GPIO_Port, DB_G_Pin, GPIO_PIN_RESET);

	osStatus_t mbmsStatus_a1 = osMutexAcquire(MBMSStatusMutexHandle, UPDATING_MUTEX_TIMEOUT );
	if(mbmsStatus_a1 == osOK) {
		mbmsStatus.System_state = SOFT_TRIP;
		osMutexRelease(MBMSStatusMutexHandle);
	}
	carState = SOFT_TRIP;
	osStatus_t Permissions_a4 = osMutexAcquire(PermissionsMutexHandle, UPDATING_MUTEX_TIMEOUT );
	if(Permissions_a4 == osOK)
	{
		// got rid of this bc what if we need to charge or smth idk....
		//mbmsPermissions.faulted = 1;
		osMutexRelease(PermissionsMutexHandle);
	}
}




void enter_CHARGING()
{
	HAL_GPIO_WritePin(DB_R_GPIO_Port, DB_R_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(DB_B_GPIO_Port, DB_B_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(DB_G_GPIO_Port, DB_G_Pin, GPIO_PIN_SET);

	osStatus_t mbmsStatus_a1 = osMutexAcquire(MBMSStatusMutexHandle, UPDATING_MUTEX_TIMEOUT );
	if(mbmsStatus_a1 == osOK) {
		mbmsStatus.System_state = CHARGING;
		osMutexRelease(MBMSStatusMutexHandle);
	}
	carState = CHARGING;
}




void enter_FULLY_OPERATIONAL()
{
	HAL_GPIO_WritePin(DB_R_GPIO_Port, DB_R_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(DB_B_GPIO_Port, DB_B_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(DB_G_GPIO_Port, DB_G_Pin, GPIO_PIN_SET);

	carState = FULLY_OPERATIONAL;

//	osStatus_t mbmsStatus_a1 = osMutexAcquire(MBMSStatusMutexHandle, UPDATING_MUTEX_TIMEOUT );
//	if(mbmsStatus_a1 == osOK) {
	// HEY DONT PUT A MUTEX AROUND THIS BC THIS FUNCTION IS NESTED IN THE MBMSTSTAUS MUTEX ALREADY
	// IN THE SYSTEM STATE STUFF !!!!
		mbmsStatus.System_state = FULLY_OPERATIONAL;
//		osMutexRelease(MBMSStatusMutexHandle);
//	}
}




/* Startup enter_SOFT_TRIPs */
void startupCheck() // change after this function is done: waitForFirstHeartbeats
{
#if test_with_CCPs
    /* Run startup gate checks in order. If any fail, enter fault. */
    if (waitForFirstHeartbeats())
    {
        enter_BPS_FAULT();   // preferred name from your header
        return;
    }
//    Check_ContactorHeartbeats();
#endif

    // dont want this to run during startup state when contactors can be closed/closing
//    if (!checkContactorsOpen() || !checkPrechargersOpen())
//    {
//    	enter_BPS_FAULT();
//        return;
//    }

    if (!startupBatteryCheck())
    {
    	enter_BPS_FAULT();
        return;
    }
}



// Add mutexs around shared variables
uint8_t waitForFirstHeartbeats()
{


	static uint8_t heartbeatFailCounter[NUM_OF_CNTR] = {0};
	uint8_t dead = 0;


	for(int i = 0; i < NUM_OF_CNTR; i++)
	{

		//heartbeat_check_count++;

		// case that a ccp heartbeat has died
		if (heartbeatFailCounter[i] > MAX_HEARTBEAT_FAILS)
		{
			osStatus_t MBMSTrip_a1 = osMutexAcquire(MBMSTripMutexHandle, UPDATING_MUTEX_TIMEOUT);
			if(MBMSTrip_a1 == osOK)
			{
				switch (i)
				{
					case LV:
						mbmsHardTrips.LV_no_heartbeat_trip = 1;
						// dead = 1;
						break;
					case MOTOR1:
						mbmsHardTrips.MT1_no_heartbeat_trip = 1;
						break;
					case MOTOR2:
						mbmsHardTrips.MT2_no_heartbeat_trip = 1;
						break;
					case ARRAY:
						mbmsHardTrips.AR_no_heartbeat_trip = 1;
						break;
					case CHARGE:
						mbmsHardTrips.CHG_no_heartbeat_trip = 1;
						break;
				}
				osMutexRelease(MBMSTripMutexHandle);
			}

			dead = 1;
			return dead;
		}

		// case that the heartbeat has reached max value
		if (previousHeartbeats[i] >= 65535)
		{ // check this logic lol
			previousHeartbeats[i] = 0;
		}

		// case that heartbeat update has timed out
		// This checks whether the heartbeat did not increase.
		osStatus_t ContactorInfo_a2 = osMutexAcquire(ContactorInfoMutexHandle, READING_MUTEX_TIMEOUT );
		if(ContactorInfo_a2 == osOK)
		{
			if(previousHeartbeats[i] >= contactorInfo[i].heartbeat)
			{
			// If the heartbeat hasn't changed for too long, the system assumes it may have stalled.
				if(((osKernelGetTickCount() - heartbeatLastUpdatedTime[i])) > CONTACTOR_HEARTBEAT_TIMEOUT)
				{ 	// where contactor_heartbeat_timeout is how often a heartbeat is sent out/recieved
					// The failure counter increases.
					heartbeatFailCounter[i]++;
				}
			}
			osMutexRelease(ContactorInfoMutexHandle);
		}

		// NOTE  TODO / FIX: lowkey this kinda wrong cuz the else is for the if statement getting mutex now..
		// can prob manually do like.. if prev hb  < contactorInfo hb

		// If the heartbeat did increase, the controller is alive. (Updates the last heartbeat time, Resets the failure counter, Stores the new heartbeat value)
		if (previousHeartbeats[i] < contactorInfo[i].heartbeat)
		{
			heartbeatLastUpdatedTime[i] = osKernelGetTickCount();
			heartbeatFailCounter[i] = 0;
			osStatus_t ContactorInfo_a3 = osMutexAcquire(ContactorInfoMutexHandle, READING_MUTEX_TIMEOUT );
			if(ContactorInfo_a3 == osOK)
			{
				previousHeartbeats[i] = (contactorInfo[i].heartbeat);
				osMutexRelease(ContactorInfoMutexHandle);
			}
		}


	}

	return dead;
}



// TODO/ PLEASE ADD the hard trips mutex here too, just the way u nromally do them if u have both mutexes type thing

uint8_t startupBatteryCheck()
{
    uint8_t pass = 1;
	osStatus_t Batteryinfo_a1 = osMutexAcquire(BatteryInfoMutexHandle, READING_MUTEX_TIMEOUT );
	osStatus_t MBMSTrip_a2 = osMutexAcquire(MBMSTripMutexHandle, UPDATING_MUTEX_TIMEOUT);
	if(Batteryinfo_a1 == osOK && MBMSTrip_a2 == osOK)
	{

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
		osMutexRelease(BatteryInfoMutexHandle);
		osMutexRelease(MBMSTripMutexHandle);
	}
	else {
		if(Batteryinfo_a1 == osOK) {
			osMutexRelease(BatteryInfoMutexHandle);
		}
		if(MBMSTrip_a2 == osOK) {
			osMutexRelease(MBMSTripMutexHandle);
		}
	}

    startup_Check_Counter++;
    return pass;
}





uint8_t checkPrechargersOpen()
{
	uint8_t pass = 1;
    for (int i = 0; i < NUM_OF_CNTR; i++)
    {
    	osStatus_t ContactorInfo_a4 = osMutexAcquire(ContactorInfoMutexHandle, READING_MUTEX_TIMEOUT );
    	if(ContactorInfo_a4 == osOK)
    	{
			/* If the precharger is reported closed, then it is NOT open. */
			if (contactorInfo[i].precharge_close == CLOSE_CONTACTOR)
			{
				pass = 0;
				osMutexRelease(ContactorInfoMutexHandle); //need this here too -m
				return pass;
			}
			osMutexRelease(ContactorInfoMutexHandle);
		}
    }
    return pass;
}





uint8_t checkContactorsOpen()
{
	uint8_t pass = 1;
    for (int i = 0; i < NUM_OF_CNTR; i++)
    {
		osStatus_t ContactorInfo_a5 = osMutexAcquire(ContactorInfoMutexHandle, READING_MUTEX_TIMEOUT );
		if(ContactorInfo_a5 == osOK)
		{
			if (contactorInfo[i].contactor_close == CLOSE_CONTACTOR)
			{
				pass = 0;
				osMutexRelease(ContactorInfoMutexHandle); // need this here too -m
				return pass;
			}
			osMutexRelease(ContactorInfoMutexHandle);
		}
    }
    return pass;
}





/* ------ Main Control Functions ----- */

void SystemStateMachine()
{
	uint8_t plugged = read_EVCC_12V_SW() == EVCC_12V_SW_ACTIVE;


	switch (mbmsStatus.System_state) // switch to mbmsStatus.systemstate but make sure u update all the states and have all the enter funcs!!!
	{
	case BOOT:
		// checking that all cntrs and pchgs r open initially
		if ( !(checkContactorsOpen() && checkPrechargersOpen()) ) {
			enter_BPS_FAULT();
			break;
		}

		//make sure these are the counters faisal used for updatEBATTERYINFO
		if((pack_info_counter >= MINIMUM_ORION_MESSAGE_RECEIVED ) && (temp_info_counter >= MINIMUM_ORION_MESSAGE_RECEIVED)
			&& (cell_voltages_counter >= MINIMUM_ORION_MESSAGE_RECEIVED))
		{
			carState = STARTUP;
			mbmsStatus.System_state = STARTUP;
		}
		break;


	case STARTUP:

		// will go to BPS_FAULT state if startup checks do not pass
		startupCheck();

		//checks MPS
		if(read_MPS() != MPS_ACTIVE)
		{
			enter_MPS_DISCONNECTED();
			break;
		}

		if(read_ESD() == ESD_ACTIVE)
		{
			osStatus_t MBMSTrip_a3 = osMutexAcquire(MBMSTripMutexHandle, UPDATING_MUTEX_TIMEOUT);
			if(MBMSTrip_a3 == osOK)
			{
				mbmsHardTrips.ESD_trip = 1;

				//CHECK HEREEE
				enter_BPS_FAULT();
			}
			osMutexRelease(MBMSTripMutexHandle);
		}
		osStatus_t MBMSStatus_a3 = osMutexAcquire(MBMSStatusMutexHandle, READING_MUTEX_TIMEOUT );
		if(MBMSStatus_a3 == osOK)
		{
			if(mbmsStatus.Startup_state == STARTUP_DONE)
			{
 				enter_FULLY_OPERATIONAL();
			}
			osMutexRelease(MBMSStatusMutexHandle);
		}
		break;


	case FULLY_OPERATIONAL:

		if(read_MPS() != MPS_ACTIVE)
		{
			enter_MPS_DISCONNECTED();
			break;
		}

		check_charging();


#if test_with_CCPs
		Check_ContactorHeartbeats();
#endif
		Update_SoftTripStruct();
		Update_TripStruct();

		break;

	case CHARGING:

		if(read_MPS() != MPS_ACTIVE)
		{
			enter_MPS_DISCONNECTED();
			break;
		}

		// if charger unplugged & allowed to discharge
		if( !plugged && (read_Discharge_EN() == DISCHARGE_ENABLE_ACTIVE))
		{
			HAL_GPIO_WritePin(_14V_Charge_EN_GPIO_Port, _14V_Charge_EN_Pin, !_14V_CHARGE_EN_ACTIVE); //Discharge THE CHARGER
			osStatus_t Permissions_a10 = osMutexAcquire(PermissionsMutexHandle, UPDATING_MUTEX_TIMEOUT );
			if(Permissions_a10 == osOK)
			{
				mbmsPermissions.charge = 0;
				osMutexRelease(PermissionsMutexHandle);
			}
		}

		// once charge cntr is opened, close 12V CAN pchg
		osStatus_t ContactorInfo_a8 = osMutexAcquire(ContactorInfoMutexHandle, READING_MUTEX_TIMEOUT );
		if(ContactorInfo_a8 == osOK)
		{
			if((contactorInfo[CHARGE].contactor_close == OPEN_CONTACTOR) &&
					(read_14V_Charge_EN() != _14V_CHARGE_EN_ACTIVE))
			{
				HAL_GPIO_WritePin(_12V_CAN_PCHG_GPIO_Port, _12V_CAN_PCHG_Pin, _12V_CAN_PCHG_ACTIVE);
			}
			osMutexRelease(ContactorInfoMutexHandle);
		}

		// once done 12V CAN pchging, close 12V CAN cntr
		if ((read_12V_CAN_State() == _12V_CAN_STATE_ACTIVE) && (read_12V_CAN_PCHG() == _12V_CAN_PCHG_ACTIVE)) {
			HAL_GPIO_WritePin(_12V_CAN_EN_GPIO_Port, _12V_CAN_EN_Pin, _12V_CAN_EN_ACTIVE);
			HAL_GPIO_WritePin(_12V_CAN_PCHG_GPIO_Port, _12V_CAN_PCHG_Pin, !(_12V_CAN_PCHG_ACTIVE));
		}

		// once 12V CAN fully enabled, enable LV & motor
		if (read_12V_CAN_EN() == _12V_CAN_EN_ACTIVE && contactorInfo[CHARGE].contactor_close != CLOSE_CONTACTOR)
		{
			osStatus_t Permissions_a11 = osMutexAcquire(PermissionsMutexHandle, UPDATING_MUTEX_TIMEOUT );
			if(Permissions_a11 == osOK)
			{
				mbmsPermissions.lv = 1;
				mbmsPermissions.motor1 = 1;
				mbmsPermissions.motor2 = 1;
				osMutexRelease(PermissionsMutexHandle);
			}
		}

		// finally, car becomes fully op
		osStatus_t ContactorInfo_a9 = osMutexAcquire(ContactorInfoMutexHandle, READING_MUTEX_TIMEOUT );
		if(ContactorInfo_a9 == osOK)
		{
			if((contactorInfo[LV].contactor_close == CLOSE_CONTACTOR) && (contactorInfo[MOTOR1].contactor_close == CLOSE_CONTACTOR)
				&& (contactorInfo[MOTOR2].contactor_close == CLOSE_CONTACTOR))
			{
				enter_FULLY_OPERATIONAL();
			}
			osMutexRelease(ContactorInfoMutexHandle);
		}
#if test_with_CCPs
		Check_ContactorHeartbeats();
#endif
		Update_SoftTripStruct();
		Update_TripStruct();

		break;

	case BPS_FAULT:
		if (read_ESD() == ESD_ACTIVE) {
			osStatus_t mbmsStatus_a16 = osMutexAcquire(MBMSStatusMutexHandle, UPDATING_MUTEX_TIMEOUT);
			if (mbmsStatus_a16 == osOK) {
				mbmsStatus.ESD = 1;
				osMutexRelease(MBMSStatusMutexHandle);
			}
		}
		Update_SoftTripStruct();
		Update_TripStruct();
		break;

	case MPS_DISCONNECTED:
		Update_SoftTripStruct();
		Update_TripStruct();
		break;

	case SOFT_TRIP:

		osStatus_t softtrip_a1 = osMutexAcquire(MBMSSoftTripMutexHandle, READING_MUTEX_TIMEOUT );
		osStatus_t Permissions_a12 = osMutexAcquire(PermissionsMutexHandle, UPDATING_MUTEX_TIMEOUT );
		if(softtrip_a1 == osOK && Permissions_a12 == osOK)
		{
			if(mbmsSoftTrips.High_volt_cell_Strip == 1)
			{
				mbmsPermissions.charge = 0;
				mbmsPermissions.array  = 0;
				mbmsPermissions.motor1 = 0;
				mbmsPermissions.motor2 = 0;

			}
			if(mbmsSoftTrips.Low_volt_cell_Strip == 1)
			{
				mbmsPermissions.motor1 = 0;
				mbmsPermissions.motor2 = 0;
				mbmsPermissions.lv = 0;
			}
			osMutexRelease(MBMSSoftTripMutexHandle);
			osMutexRelease(PermissionsMutexHandle);
		}
		else {
			if(softtrip_a1 == osOK) {
				osMutexRelease(MBMSSoftTripMutexHandle);
			}
			if(Permissions_a12 == osOK) {
				osMutexRelease(PermissionsMutexHandle);
			}
		}


		if(read_MPS() != MPS_ACTIVE)
		{
			enter_MPS_DISCONNECTED();
			break;
		}

		check_charging();

#if test_with_CCPs
		Check_ContactorHeartbeats();
#endif
		Update_TripStruct();
		Update_SoftTripStruct();
		break;
	}
}


void check_charging() {

	uint8_t plugged = read_EVCC_12V_SW() == EVCC_12V_SW_ACTIVE;

	if (!mbmsSoftTrips.High_volt_cell_Strip) {
		// if EVCC_12V_Sw is enabled and OBMS sends charge_en active
		if(plugged && (read_Charge_EN() == CHARGE_ENABLE_ACTIVE) )
		{
			osStatus_t Permissions_a5 = osMutexAcquire(PermissionsMutexHandle, UPDATING_MUTEX_TIMEOUT );
			if(Permissions_a5 == osOK)
			{
				mbmsPermissions.lv = 0;
				mbmsPermissions.motor1 = 0;
				mbmsPermissions.motor2 = 0;

				osMutexRelease(PermissionsMutexHandle);
			}
			if (read_12V_CAN_EN() == _12V_CAN_EN_ACTIVE){
				HAL_GPIO_WritePin(_12V_CAN_EN_GPIO_Port, _12V_CAN_EN_Pin, GPIO_PIN_RESET); //12V CAN Disabled

			}
		}
		else {
			HAL_GPIO_WritePin(_14V_Charge_EN_GPIO_Port, _14V_Charge_EN_Pin, !CHARGE_ENABLE_ACTIVE);
			osStatus_t Permissions_a6 = osMutexAcquire(PermissionsMutexHandle, UPDATING_MUTEX_TIMEOUT );
			if(Permissions_a6 == osOK)
			{
				mbmsPermissions.charge = 0;
				osMutexRelease(PermissionsMutexHandle);
			}
			if((contactorInfo[CHARGE].contactor_close != CLOSE_CONTACTOR)) {
				osStatus_t Permissions_a7 = osMutexAcquire(PermissionsMutexHandle, UPDATING_MUTEX_TIMEOUT );
				if(Permissions_a7 == osOK)
				{
					mbmsPermissions.lv = 1;
					mbmsPermissions.motor1 = 1;
					mbmsPermissions.motor2 = 1;

					osMutexRelease(PermissionsMutexHandle);
				}
			}
			if ((read_12V_CAN_State() != _12V_CAN_STATE_ACTIVE) &&
					(read_12V_CAN_PCHG() != _12V_CAN_PCHG_ACTIVE) ) {
				HAL_GPIO_WritePin(_12V_CAN_PCHG_GPIO_Port, _12V_CAN_PCHG_Pin, _12V_CAN_PCHG_ACTIVE);

			}
			if ((read_12V_CAN_State() == _12V_CAN_STATE_ACTIVE) && (read_12V_CAN_PCHG() == _12V_CAN_PCHG_ACTIVE)
					&& read_12V_CAN_EN() != _12V_CAN_EN_ACTIVE) {
				HAL_GPIO_WritePin(_12V_CAN_EN_GPIO_Port, _12V_CAN_EN_Pin, _12V_CAN_EN_ACTIVE);
				HAL_GPIO_WritePin(_12V_CAN_PCHG_GPIO_Port, _12V_CAN_PCHG_Pin, !(_12V_CAN_PCHG_ACTIVE));
			}
		}

		// TODO not a todo but this looks rly good u should do this for when u have nested stuff like how u do
		// waiting to have both mutexes before executing the section that interacts with the multiple variables
		osStatus_t ContactorInfo_a6 = osMutexAcquire(ContactorInfoMutexHandle, READING_MUTEX_TIMEOUT );
		osStatus_t Permissions_a8 = osMutexAcquire(PermissionsMutexHandle, UPDATING_MUTEX_TIMEOUT );
		if(ContactorInfo_a6 == osOK && Permissions_a8 == osOK)
		{
			if(plugged && (contactorInfo[LV].contactor_close == OPEN_CONTACTOR) && (contactorInfo[MOTOR1].contactor_close == OPEN_CONTACTOR)
				&& (contactorInfo[MOTOR2].contactor_close == OPEN_CONTACTOR))
			{
				HAL_GPIO_WritePin(_14V_Charge_EN_GPIO_Port, _14V_Charge_EN_Pin, CHARGE_ENABLE_ACTIVE); //ENABLE THE CHARGER
				mbmsPermissions.charge = 1;
			}
			osMutexRelease(ContactorInfoMutexHandle);
			osMutexRelease(PermissionsMutexHandle);
		}
		else {
			if(ContactorInfo_a6 == osOK) {
				osMutexRelease(ContactorInfoMutexHandle);
			}
			if(Permissions_a8 == osOK) {
				osMutexRelease(PermissionsMutexHandle);
			}
		}

		// i lowkey split these into 2 chunks tho heh
		osStatus_t ContactorInfo_a7 = osMutexAcquire(ContactorInfoMutexHandle, READING_MUTEX_TIMEOUT );
		osStatus_t Permissions_a9 = osMutexAcquire(PermissionsMutexHandle, UPDATING_MUTEX_TIMEOUT );
		if(ContactorInfo_a7 == osOK && Permissions_a9 == osOK)
		{

			if(plugged && (contactorInfo[CHARGE].contactor_close == CLOSE_CONTACTOR))
			{
				// CHECK NESTED MUTEX STUFF
				enter_CHARGING();
			}
			osMutexRelease(ContactorInfoMutexHandle);
			osMutexRelease(PermissionsMutexHandle);

		}
		else {
			if(ContactorInfo_a7 == osOK) {
				osMutexRelease(ContactorInfoMutexHandle);
			}
			if(Permissions_a9 == osOK) {
				osMutexRelease(PermissionsMutexHandle);
			}
		}
	}

}

void Control_Contactors()
{

	uint8_t contactorClosing = false;

	//check if any of the contactors are closed
	for (int i = 0; i < NUM_OF_CNTR; i++)
	{
		osStatus_t ContactorInfo_a10 = osMutexAcquire(ContactorInfoMutexHandle, READING_MUTEX_TIMEOUT );
		if(ContactorInfo_a10 == osOK)
		{
			if (contactorInfo[i].contactor_closing == CLOSING_CONTACTOR)
			{
				contactorClosing = true;
				osMutexRelease(ContactorInfoMutexHandle);
				break;
			}
		}
		osMutexRelease(ContactorInfoMutexHandle);
	}


	// TODO not a todo but another example of how u handled nested mutexes which is rly nice btw !!!!
	osStatus_t ContactorInfo_a11 = osMutexAcquire(ContactorInfoMutexHandle, READING_MUTEX_TIMEOUT );
	osStatus_t Permissions_a11 = osMutexAcquire(PermissionsMutexHandle, READING_MUTEX_TIMEOUT );
	osStatus_t MBMSStatus_a4 = osMutexAcquire(MBMSStatusMutexHandle, READING_MUTEX_TIMEOUT );
	osStatus_t ContactorCommand_a1 = osMutexAcquire(ContactorCommandMutexHandle, UPDATING_MUTEX_TIMEOUT );
	if(ContactorInfo_a11 == osOK && Permissions_a11 == osOK && MBMSStatus_a4 == osOK && ContactorCommand_a1 ==osOK)
	{
		//if no contactors are in the process of closing and the battery is nor in fault tpe state (MPS,BPS)
		if (!contactorClosing && !mbmsPermissions.faulted)
		{
			if((mbmsPermissions.lv) && (contactorInfo[LV].contactor_close != CLOSE_CONTACTOR) && (mbmsStatus.discharge_enable == DISCHARGE_ENABLE_ACTIVE))
			{
				contactorCommand.LV = CLOSE_CONTACTOR;
			}
			else if ((mbmsPermissions.motor1) && (contactorInfo[MOTOR1].contactor_close != CLOSE_CONTACTOR ) && (mbmsStatus.discharge_enable == DISCHARGE_ENABLE_ACTIVE))
			{
				contactorCommand.motor1 = CLOSE_CONTACTOR;
			}
			else if ((mbmsPermissions.motor2) && (contactorInfo[MOTOR2].contactor_close != CLOSE_CONTACTOR ) && (mbmsStatus.discharge_enable == DISCHARGE_ENABLE_ACTIVE))
			{
				contactorCommand.motor2 = CLOSE_CONTACTOR;
			}
			else if ((mbmsPermissions.array) && (contactorInfo[ARRAY].contactor_close != CLOSE_CONTACTOR) && (mbmsStatus.charge_enable == CHARGE_ENABLE_ACTIVE))
			{
				contactorCommand.array = CLOSE_CONTACTOR;
			}
			else if ((mbmsPermissions.charge) && (contactorInfo[CHARGE].contactor_close != CLOSE_CONTACTOR) && (mbmsStatus.charge_enable == CHARGE_ENABLE_ACTIVE))
			{
				contactorCommand.charge = CLOSE_CONTACTOR;
			}
		}
		osMutexRelease(ContactorInfoMutexHandle);
		osMutexRelease(PermissionsMutexHandle);
		osMutexRelease(MBMSStatusMutexHandle);
		osMutexRelease(ContactorCommandMutexHandle);
	}
	//////////////////////////////////////////////
	///////////////////////////////////////////////
	// NEED TO CHECK ALL NESTED MUTEX STUFF ABOVE THIS LINE !!!!!
	else {
		if(ContactorInfo_a11 == osOK) {
			osMutexRelease(ContactorInfoMutexHandle);
		}
		if(Permissions_a11 == osOK) {
			osMutexRelease(PermissionsMutexHandle);
		}
		if(MBMSStatus_a4 == osOK) {
			osMutexRelease(MBMSStatusMutexHandle);
		}
		if(ContactorCommand_a1 ==osOK) {
			osMutexRelease(ContactorCommandMutexHandle);
		}
	}


	osStatus_t Permissions_a12 = osMutexAcquire(PermissionsMutexHandle, READING_MUTEX_TIMEOUT );
	osStatus_t ContactorCommand_a2 = osMutexAcquire(ContactorCommandMutexHandle, UPDATING_MUTEX_TIMEOUT );
	if(Permissions_a12 == osOK && ContactorCommand_a2 == osOK)
	{
		if (!mbmsPermissions.lv){
			contactorCommand.LV = OPEN_CONTACTOR;
		}

		if (!mbmsPermissions.motor1){
			contactorCommand.motor1 = OPEN_CONTACTOR;
		}

		if (!mbmsPermissions.motor2){
			contactorCommand.motor2 = OPEN_CONTACTOR;
		}

		if (!mbmsPermissions.array){
			contactorCommand.array = OPEN_CONTACTOR;
		}

		if (!mbmsPermissions.charge){
			contactorCommand.charge = OPEN_CONTACTOR;
		}
		osMutexRelease(PermissionsMutexHandle);
		osMutexRelease(ContactorCommandMutexHandle);
	}
	else {
		if(Permissions_a12 == osOK) {
			osMutexRelease(PermissionsMutexHandle);
		}
		if(ContactorCommand_a2 == osOK) {
			osMutexRelease(ContactorCommandMutexHandle);
		}
	}
}



/*----- Checking for Trips & Strips & Dead Heartbeats Functions -----*/
void Update_TripStruct()
{
	static uint8_t BPS_Fault = 0;

	osStatus_t MBMSTrip_a4 = osMutexAcquire(MBMSTripMutexHandle, UPDATING_MUTEX_TIMEOUT);
	osStatus_t ContactorInfo_a12 = osMutexAcquire(ContactorInfoMutexHandle, READING_MUTEX_TIMEOUT );
	osStatus_t Batteryinfo_a2 = osMutexAcquire(BatteryInfoMutexHandle, READING_MUTEX_TIMEOUT );
	if(MBMSTrip_a4 == osOK && ContactorInfo_a12 == osOK && Batteryinfo_a2 ==osOK)
	{
		// Checking High Current & Reverse Current Trips
		if(batteryInfo.packCurrent > HARD_MAX_COMMON_CONTACTOR_CURRENT )
		{
			mbmsHardTrips.CMN_high_cur_trip = 1;
			BPS_Fault = 1;
		}

		if(abs(contactorInfo[MOTOR1].line_current) > HARD_MAX_MOTORS_CONTACTOR_CURRENT)
		{
			mbmsHardTrips.MT1_high_cur_trip = 1;
			BPS_Fault = 1;
		}

		if(abs(contactorInfo[MOTOR2].line_current) > HARD_MAX_MOTORS_CONTACTOR_CURRENT)
		{
			mbmsHardTrips.MT2_high_cur_trip = 1;
			BPS_Fault = 1;
		}

		if(abs(contactorInfo[ARRAY].line_current) > HARD_MAX_ARRAY_CONTACTOR_CURRENT)
		{
			mbmsHardTrips.AR_high_cur_trip = 1;
			BPS_Fault = 1;
		}

		if(abs(contactorInfo[LV].line_current) > HARD_MAX_LV_CONTACTOR_CURRENT)
		{
			mbmsHardTrips.LV_high_cur_trip = 1;
			BPS_Fault = 1;
		}

		if(abs(contactorInfo[CHARGE].line_current) > HARD_MAX_CHARGE_CONTACTOR_CURRENT)
		{
			mbmsHardTrips.CHG_high_cur_trip = 1;
			BPS_Fault = 1;
		}

		// Checking reverse current trips for charge and lv
		if(contactorInfo[CHARGE].line_current > 0 || contactorInfo[LV].line_current < 0)
		{
			mbmsHardTrips.Reverse_cur_trip = 1;
			BPS_Fault = 1;
		}
		osMutexRelease(MBMSTripMutexHandle);
		osMutexRelease(ContactorInfoMutexHandle);
		osMutexRelease(BatteryInfoMutexHandle);
	}
	else {
		if(MBMSTrip_a4 == osOK) {
			osMutexRelease(MBMSTripMutexHandle);
		}
		if(ContactorInfo_a12 == osOK) {
			osMutexRelease(ContactorInfoMutexHandle);
		}
		if(Batteryinfo_a2 ==osOK) {
			osMutexRelease(BatteryInfoMutexHandle);
		}
	}


	osStatus_t MBMSTrip_a5 = osMutexAcquire(MBMSTripMutexHandle, UPDATING_MUTEX_TIMEOUT);
	osStatus_t Batteryinfo_a3 = osMutexAcquire(BatteryInfoMutexHandle, READING_MUTEX_TIMEOUT );
	osStatus_t MBMSStatus_a5 = osMutexAcquire(MBMSStatusMutexHandle, READING_MUTEX_TIMEOUT );
	if(MBMSTrip_a5 == osOK && Batteryinfo_a3 ==osOK && MBMSStatus_a5 == osOK)
		{
			// Checking Battery Related Trips!!
			if(batteryInfo.highCellVoltage > HARD_MAX_CELL_VOLTAGE)
			{
				mbmsHardTrips.High_volt_cell_trip = 1;
				BPS_Fault = 1;
			}

			if(batteryInfo.lowCellVoltage < HARD_MIN_CELL_VOLTAGE)
			{
				mbmsHardTrips.Low_volt_cell_trip = 1;
				BPS_Fault = 1;
			}

			if(batteryInfo.highTemp > HARD_MAX_TEMP)
			{
				mbmsHardTrips.High_temp_trip = 1;
				BPS_Fault = 1;
			}

			if(batteryInfo.lowTemp < HARD_MIN_TEMP)
			{
				mbmsHardTrips.Low_temp_trip = 1;
				BPS_Fault = 1;
			}

	    	// Checking Missing Orion Messages Trip
			//if orion CAN msg wasnt received recently, trip set
			if(!(mbmsStatus.OBMS_CAN_RR))
			{
				mbmsHardTrips.OBMS_msg_timeout_trip = 1;
				BPS_Fault = 1;
			}

		osMutexRelease(MBMSTripMutexHandle);
		osMutexRelease(BatteryInfoMutexHandle);
		osMutexRelease(MBMSStatusMutexHandle);
	}
	else {
		if(MBMSTrip_a5 == osOK) {
			osMutexRelease(MBMSTripMutexHandle);
		}
		if(Batteryinfo_a3 ==osOK) {
			osMutexRelease(BatteryInfoMutexHandle);
		}
		if(MBMSStatus_a5 == osOK) {
			osMutexRelease(MBMSStatusMutexHandle);
		}
	}

#if test_with_CCPs

	// Checking contactor disconnected & connected unexpectedly trip
	osStatus_t ContactorCommand_a3 = osMutexAcquire(ContactorCommandMutexHandle, READING_MUTEX_TIMEOUT);
	osStatus_t ContactorInfo_a13 = osMutexAcquire(ContactorInfoMutexHandle, READING_MUTEX_TIMEOUT );
	osStatus_t MBMSTrip_a6 = osMutexAcquire(MBMSTripMutexHandle, UPDATING_MUTEX_TIMEOUT);
	if (ContactorCommand_a3 == osOK && ContactorInfo_a13 == osOK && MBMSTrip_a6 == osOK)
	{
		/* Contactor disconnected unexpectedly */
		/* To check, we compare a minimum current draw with the state of the contactor */
		if(((contactorCommand.motor1 == CLOSE_CONTACTOR) 	 && 	(abs(contactorInfo[MOTOR1].line_current) < NO_CURRENT_THRESHOLD)) 	 ||
		  ((contactorCommand.motor2 == CLOSE_CONTACTOR) 	 && 	(abs(contactorInfo[MOTOR2].line_current) < NO_CURRENT_THRESHOLD)) 	 ||
		  ((contactorCommand.array == CLOSE_CONTACTOR) 		 && 	(abs(contactorInfo[ARRAY].line_current) < NO_CURRENT_THRESHOLD)) 	 ||
		  ((contactorCommand.LV == CLOSE_CONTACTOR) 		 && 	(abs(contactorInfo[LV].line_current) < NO_CURRENT_THRESHOLD)) 		 ||
		  ((contactorCommand.charge == CLOSE_CONTACTOR) 	 && 	(abs(contactorInfo[CHARGE].line_current) < NO_CURRENT_THRESHOLD))
		)
		{
			mbmsHardTrips.CNTR_disconnect_trip = 1;
			BPS_Fault = 1;

		}

		/* Contactor connected unexpectedly trip */
		if(((contactorCommand.motor1 == OPEN_CONTACTOR) 	&& 	(abs(contactorInfo[MOTOR1].line_current) >= NO_CURRENT_THRESHOLD))	||
		  ((contactorCommand.motor2 == OPEN_CONTACTOR) 	&& 	(abs(contactorInfo[MOTOR2].line_current) >= NO_CURRENT_THRESHOLD))		||
		  ((contactorCommand.array == OPEN_CONTACTOR) 	&& 	(abs(contactorInfo[ARRAY].line_current) >= NO_CURRENT_THRESHOLD)) 	 	||
		  ((contactorCommand.LV == OPEN_CONTACTOR) 		&& 	(abs(contactorInfo[LV].line_current) >= NO_CURRENT_THRESHOLD)) 		 	||
		  ((contactorCommand.charge == OPEN_CONTACTOR) 	&& 	(abs(contactorInfo[CHARGE].line_current) >= NO_CURRENT_THRESHOLD)))
		{
			mbmsHardTrips.CNTR_connect_trip = 1;
			BPS_Fault = 1;
		}

		/* Here, it is also a contactor connected unexpectedly trip if the contactor won't open when told to */
		for(int i = 0; i < NUM_OF_CNTR; i++)
		{
			if(contactorInfo[i].contactor_opening_error == 1 )
			{
				mbmsHardTrips.CNTR_connect_trip = 1;
				BPS_Fault = 1;
			}
		}

		osMutexRelease(ContactorCommandMutexHandle);
		osMutexRelease(ContactorInfoMutexHandle);
		osMutexRelease(MBMSTripMutexHandle);
	}
	else {
		if(ContactorCommand_a3 == osOK) {
			osMutexRelease(ContactorCommandMutexHandle);
		}
		if(ContactorInfo_a13 == osOK ) {
			osMutexRelease(ContactorInfoMutexHandle);
		}
		if(MBMSTrip_a6 == osOK) {
			osMutexRelease(MBMSTripMutexHandle);
		}
	}
#endif

	osStatus_t MBMSTrip_a7 = osMutexAcquire(MBMSTripMutexHandle, UPDATING_MUTEX_TIMEOUT);
	if(MBMSTrip_a7 == osOK)
	{
		// Check ESD
		if (read_ESD() == ESD_ACTIVE)
		{
			mbmsHardTrips.ESD_trip = 1;
			BPS_Fault = 1;
		}
		osMutexRelease(MBMSTripMutexHandle);
	}

	// check main & common cntr !!!
	if ((read_Common_CNTR_Aux() != COMMON_CNTR_ACTIVE) || (read_Main_CNTR_Aux() != MAIN_CNTR_AUX_ACTIVE)) {
		// note there is no displayed fault for this :C
		// bc technically if either of these r open hardware shuld do some faulting process for us....
		BPS_Fault = 1;
		main_cmn_cntr_trip = 1;
	}

	// Finally, if there were any trips, go to BPS FAULT state!!!!
	if(BPS_Fault)
	{
		enter_BPS_FAULT();
	}

}




void Update_SoftTripStruct()
{
	uint8_t trip = 0;

	osStatus_t MBMSStrip_a1 = osMutexAcquire(MBMSSoftTripMutexHandle, UPDATING_MUTEX_TIMEOUT);
	osStatus_t batteryInfo_a1 = osMutexAcquire(BatteryInfoMutexHandle, READING_MUTEX_TIMEOUT);

	if (MBMSStrip_a1 == osOK && batteryInfo_a1 == osOK)
	{

		if (batteryInfo.highCellVoltage > SOFT_MAX_CELL_VOLTAGE)
		{
			mbmsSoftTrips.High_volt_cell_Strip = 1;
			trip = 1;
		}

		if (batteryInfo.lowCellVoltage < SOFT_MIN_CELL_VOLTAGE)
		{
			mbmsSoftTrips.Low_volt_cell_Strip = 1;
			// if its low cell volt trup, need to charge car, every time try to charge will go back
			// to this soft state sooooooooo dont make it go into
			// soft twip state but yk will still say it in this soft twip struct so we knows !!! :D
			if (mbmsStatus.System_state != CHARGING) {
				trip = 1;
			}
		}

		if (batteryInfo.highTemp > SOFT_MAX_TEMP)
		{
			mbmsSoftTrips.High_temp_Strip = 1;
			trip = 1;
		}

		if (batteryInfo.lowTemp < SOFT_MIN_TEMP)
		{
			mbmsSoftTrips.Low_temp_Strip = 1;
			trip = 1;
		}

		if (batteryInfo.packCurrent > SOFT_MAX_COMMON_CONTACTOR_CURRENT)
		{
			mbmsSoftTrips.CMN_high_cur_Strip = 1;
			trip = 1;
		}

		osMutexRelease(BatteryInfoMutexHandle);
		osMutexRelease(MBMSSoftTripMutexHandle);
	}
	else {
		if(batteryInfo_a1 == osOK) {
			osMutexRelease(BatteryInfoMutexHandle);
		}
		if(MBMSStrip_a1 == osOK) {
			osMutexRelease(MBMSSoftTripMutexHandle);
		}
	}


	/* Checking contactor high-current soft trips */
	osStatus_t MBMSStrip_a2 = osMutexAcquire(MBMSSoftTripMutexHandle, UPDATING_MUTEX_TIMEOUT);
	osStatus_t contactorInfo_a1 = osMutexAcquire(ContactorInfoMutexHandle, READING_MUTEX_TIMEOUT);
	osStatus_t batteryInfo_a2 = osMutexAcquire(BatteryInfoMutexHandle, READING_MUTEX_TIMEOUT);

	if (contactorInfo_a1 == osOK && MBMSStrip_a2 == osOK && batteryInfo_a2 == osOK)
	{
		if (abs(contactorInfo[MOTOR1].line_current) > SOFT_MAX_MOTORS_CONTACTOR_CURRENT)
		{
			mbmsSoftTrips.MT1_high_cur_Strip = 1;
			trip = 1;
		}

		if (abs(contactorInfo[MOTOR2].line_current) > SOFT_MAX_MOTORS_CONTACTOR_CURRENT)
		{
			mbmsSoftTrips.MT2_high_cur_Strip = 1;
			trip = 1;
		}

		if (abs(contactorInfo[ARRAY].line_current) > SOFT_MAX_ARRAY_CONTACTOR_CURRENT)
		{
			mbmsSoftTrips.AR_high_cur_Strip = 1;
			trip = 1;
		}

		if (abs(contactorInfo[LV].line_current) > SOFT_MAX_LV_CONTACTOR_CURRENT)
		{
			mbmsSoftTrips.LV_high_cur_Strip = 1;
			trip = 1;
		}

		if (abs(contactorInfo[CHARGE].line_current) > SOFT_MAX_CHARGE_CONTACTOR_CURRENT)
		{
			mbmsSoftTrips.CHG_high_cur_Strip = 1;
			trip = 1;
		}

		if (abs(batteryInfo.packCurrent) > SOFT_MAX_COMMON_CONTACTOR_CURRENT)
		{
			mbmsSoftTrips.CMN_high_cur_Strip = 1;
		}

		// TODO need to check common current (pack current from orion)

		osMutexRelease(ContactorInfoMutexHandle);
		osMutexRelease(MBMSSoftTripMutexHandle);
		osMutexRelease(BatteryInfoMutexHandle);
	}
	else {
		if(contactorInfo_a1 == osOK) {
			osMutexRelease(ContactorInfoMutexHandle);
		}
		if(MBMSStrip_a2 == osOK) {
			osMutexRelease(MBMSSoftTripMutexHandle);
		}
		if(batteryInfo_a2 == osOK) {
			osMutexRelease(BatteryInfoMutexHandle);
		}
	}

	if (trip == 1)
	{
		enter_SOFT_TRIP();
	}
}




void Check_ContactorHeartbeats()
{

	static uint8_t BPS_Fault = 0;

	for (int i = 0; i < NUM_OF_CNTR; i++)
	{
		// Case where the heartbeat counter reaches its max value
		if (previousHeartbeats[i] >= 65535) // 2 bytes
		{
			previousHeartbeats[i] = 0;
		}

		osStatus_t contactorInfo_a1 = osMutexAcquire(ContactorInfoMutexHandle, READING_MUTEX_TIMEOUT); // HELEYNA CHECK THIS

		if (contactorInfo_a1 == osOK)
		{
			// Check if the heartbeat has not increased since the last check
			if (previousHeartbeats[i] >= contactorInfo[i].heartbeat)
			{
				// Calculate how long this heartbeat has been unchanged.
				uint32_t difference_ticks = osKernelGetTickCount() - heartbeatLastUpdatedTime[i];


				float difference_ms = (float) difference_ticks; // TODO smth sketchy check over this

				// If the heartbeat is stuck past the timeout, set the matching hard trip
				if (difference_ms > CONTACTOR_HEARTBEAT_TIMEOUT)
				{
					// SUSPISCIOUS MUTEX STUFF HERE
					/////or maybe its ok idk look at it
					osMutexRelease(ContactorInfoMutexHandle);

					osStatus_t tripAcquire = osMutexAcquire(MBMSTripMutexHandle, UPDATING_MUTEX_TIMEOUT);

					// Set the no-heartbeat hard trip for the failed contactor board.
					if (tripAcquire == osOK)
					{
						switch (i)
						{
							case LV:
								mbmsHardTrips.LV_no_heartbeat_trip = 1;
								BPS_Fault = 1;
								break;

							case MOTOR1:
								mbmsHardTrips.MT1_no_heartbeat_trip = 1;
								BPS_Fault = 1;
								break;

							case MOTOR2:
								mbmsHardTrips.MT2_no_heartbeat_trip = 1;
								break;

							case ARRAY:
								mbmsHardTrips.AR_no_heartbeat_trip = 1;
								BPS_Fault = 1;
								break;

							case CHARGE:
								mbmsHardTrips.CHG_no_heartbeat_trip = 1;
								BPS_Fault = 1;
								break;

							default:
								break;
						}

						osMutexRelease(MBMSTripMutexHandle);
						BPS_Fault = 1;
					}
				}
				else
				{
					osMutexRelease(ContactorInfoMutexHandle);
				}
			}
			else
			{
				// Heartbeat increased, so update the last-seen time and value.
				heartbeatLastUpdatedTime[i] = osKernelGetTickCount();
				previousHeartbeats[i] = contactorInfo[i].heartbeat;

				osMutexRelease(ContactorInfoMutexHandle);
			}
		} // end is osok mutex stuff
	}
	// Enter BPS fault state if any contactor heartbeat timed out.
	if (BPS_Fault)
	{
		enter_BPS_FAULT();
	}
}







// Had to use AI for these bottom functions:
void clear_Trips()
{
	osStatus_t MBMSTrip_a8 = osMutexAcquire(MBMSTripMutexHandle, UPDATING_MUTEX_TIMEOUT);
	if(MBMSTrip_a8 == osOK)
	{
		/* Reset all hard trip flags to 0 */
		memset(&mbmsHardTrips, 0, sizeof(MBMS_Hard_Trips));
	}
	osMutexRelease(MBMSTripMutexHandle);
}





void clear_SoftTrips()
{
    /* Reset all soft battery trip flags */
	osStatus_t softtrip_a2 = osMutexAcquire(MBMSSoftTripMutexHandle, UPDATING_MUTEX_TIMEOUT );
	if(softtrip_a2 == osOK)
	{
		memset(&mbmsSoftTrips, 0, sizeof(MBMS_Soft_Trips));
		osMutexRelease(MBMSSoftTripMutexHandle);
	}
}





/* ------ Updating Information Functions ------ */

void Update_ContactorInfoStruct() {
	CANmsg contactorMsg;

    osStatus_t status = osMessageQueueGet(ContactorQueueHandle, &contactorMsg, NULL, 0); // Take from que and put into struct
    if (status != osOK) {
        return; // no new message
    }
    // defines are in CAN.h for mask and ID
    uint32_t extID = contactorMsg.extendedID;

    // if the msg is a contactor info msg
    if ((extID & CNTR_MSG_MASK) == CONTACTOR_ID)
    {
    	uint8_t contactor_idx = extID - CONTACTOR_ID;

    	uint8_t *data = contactorMsg.data;

		uint8_t 	prechargerClosed   		= (data[0] >> 0) & 0x1;
		uint8_t 	prechargerClosing  		= (data[0] >> 1) & 0x1;
		uint8_t 	prechargerError    		= (data[0] >> 2) & 0x1;
		uint8_t 	contactorClosed    		= (data[0] >> 3) & 0x1;
		uint8_t 	contactorClosing   		= (data[0] >> 4) & 0x1;
		uint8_t 	contactorError     		= (data[0] >> 5) & 0x1;
		uint8_t 	contactorOpeningError 	= (data[0] >> 6) & 0x1;
		if(extID == 0x214) {
			uint8_t poop = 0;
		}
		int16_t lineCurrent = 0;
		uint16_t lineCurrent1 = (((data[0] & 0x80) >> 7) | (data[1] << 1) | ((data[2] & 0x07) << 9)); // extract bits 7 to 18
		uint16_t signed_bit = lineCurrent1 & (0x1 << 11);
		lineCurrent1 &= 0x07ff;
		if(signed_bit) {
			lineCurrent = lineCurrent1 * -1;
		}
		else {
			lineCurrent = lineCurrent1;
		}
//		lineCurrent = lineCurrent & (0x87ff);
		lineCurrent /= 10.0;

		int16_t chargeCurrent = 0;
		uint16_t 	chargeCurrent1 			= (((data[2] & 0xF8) >> 3) | ((data[3] & 0x7F) >> 6)); // extract bits 19 to 30   // in tenths of an Amp
		uint16_t signed_bit1 = chargeCurrent1 & (0x1 << 11);
		chargeCurrent1 &= 0x07ff;
		if(signed_bit1) {
			chargeCurrent = chargeCurrent1 * -1;
		}
		else {
			chargeCurrent = chargeCurrent1;
		}


		if(lineCurrent > 1000)
		{
			chargeCurrent = 5;
		}

		osStatus_t ContactorInfo_a14 = osMutexAcquire(ContactorInfoMutexHandle, UPDATING_MUTEX_TIMEOUT );
		if(ContactorInfo_a14 == osOK)
		{
			contactorInfo[contactor_idx].precharge_close = prechargerClosed;
			contactorInfo[contactor_idx].precharge_closing = prechargerClosing;
			contactorInfo[contactor_idx].precharge_error = prechargerError;
			contactorInfo[contactor_idx].contactor_close = contactorClosed;
			contactorInfo[contactor_idx].contactor_closing = contactorClosing;
			contactorInfo[contactor_idx].contactor_error = contactorError;
			contactorInfo[contactor_idx].contactor_opening_error = contactorOpeningError;
			contactorInfo[contactor_idx].line_current = lineCurrent;
			contactorInfo[contactor_idx].charge_current = chargeCurrent;
			osMutexRelease(ContactorInfoMutexHandle);
		}

    }

    // This is where we split the messages into heartbeat or board
    // if the msg is a contactor heartbeat msg. We split them again into which heartbeat CCP it is
    else
    {
    	uint8_t contactor_idx = extID - CONTACTOR_HEARTBEAT; // get index (which contactor it is)
    	uint16_t new_heartbeat = contactorMsg.data[0] + (contactorMsg.data[1] << 8);
    	osStatus_t ContactorInfo_a15 = osMutexAcquire(ContactorInfoMutexHandle, UPDATING_MUTEX_TIMEOUT );
    	if(ContactorInfo_a15 == osOK)
    	{
    		contactorInfo[contactor_idx].heartbeat = new_heartbeat;
    		osMutexRelease(ContactorInfoMutexHandle);
    	}
    }

    return;
}



void Update_DCDCStackStruct(void)
{

	osStatus_t DCDCStack_a1 = osMutexAcquire(DCDCStackMutexHandle, UPDATING_MUTEX_TIMEOUT );
	if(DCDCStack_a1 == osOK)
	{
		dcdc_stack.DCDC1_en = read_DCDC1_EN();
		dcdc_stack._14V_Charge_EN = read_14V_Charge_EN();
		dcdc_stack.nDCDC_Fault = read_nDCDC_Fault();
		dcdc_stack._12V_Critical_Fault = read_12V_Critical_Fault();
		dcdc_stack._14V_Charger_Fault = read_14V_Charger_Fault();
		dcdc_stack._12V_Critical_UC = read_12V_Critical_UC();
		osMutexRelease(DCDCStackMutexHandle);
	}
	osStatus_t mbmsStatus_a1 = osMutexAcquire(MBMSStatusMutexHandle, UPDATING_MUTEX_TIMEOUT);
	if (mbmsStatus_a1 == osOK) {
		mbmsStatus.MPS = read_MPS();
		osMutexRelease(MBMSStatusMutexHandle);
	}

}


// TODO the logic of this function got messed up w the mutexes
// try and read thru it and understand why its wrong now
// basically the else if (temp and cell volts) was meant for the if statement block checking if its pack info
// but now that you added the mutex , u also moved the next the else if blocks into
// this section they shouldnt be in..... (they are inside of pack info section now????)
// they are now the else if for whether or not u got the battery info mutex which is incorrect lol
// A way to fix, would be to move it back to what it was and get the battery info mutex
// 3 separate times, once in each if/else if block !!!
// this is also good to keep the cs small


// This function reads battery-related CAN messages from a queue
// and updates the batteryInfo struct accordingly
void Update_BatteryInfoStruct(void) // updating Orion / battery info struct
{
    CANmsg batteryMsg;  // Variable to store incoming CAN message


    // Try to get a message from the queue
    osStatus_t status = osMessageQueueGet(BatteryQueueHandle, &batteryMsg, NULL, 0);

    // If a message was successfully received
    if (status == osOK)
    {
        // Reset timeout counter since we got a message
    	missingOBMS_MsgCounter = 0;
    	osStatus_t MBMSStatus_a6 = osMutexAcquire(MBMSStatusMutexHandle, UPDATING_MUTEX_TIMEOUT );
    	if(MBMSStatus_a6 == osOK)
    	{
        // Indicate CAN communication is healthy
    		mbmsStatus.OBMS_CAN_RR = 1;
    		//read charge/discharge en from obms
    		mbmsStatus.charge_enable = read_Charge_EN();
    		mbmsStatus.discharge_enable = read_Discharge_EN();
    		osMutexRelease(MBMSStatusMutexHandle);
    	}


        // Pointer to the raw data bytes in the CAN message
        uint8_t *data = batteryMsg.data;

        // Check if this message contains pack-level information
        if (batteryMsg.extendedID == PACK_INFO_ID)
        {

        	osStatus_t Batteryinfo_a4 = osMutexAcquire(BatteryInfoMutexHandle, UPDATING_MUTEX_TIMEOUT );
        	if(Batteryinfo_a4 == osOK)
        	{
        		pack_info_counter++;
				// Ensure message has enough bytes
				if (batteryMsg.DLC >= 8) // ?? its ok -m
				{
					// Extract pack current (2 bytes, scaled by 10)
					batteryInfo.packCurrent  = (float)((uint16_t)(data[0] | (data[1] << 8))) / 10.0f;

					// Extract pack voltage (2 bytes, scaled by 10)
					batteryInfo.packVoltage  = (float)((uint16_t)(data[2] | (data[3] << 8))) / 10.0f;

					// Extract state of charge (1 byte, scaled by 2)
					batteryInfo.packSOC      = (float)(data[4]) / 2.0f;

					// Extract amp-hours (2 bytes, scaled by 10)
					batteryInfo.packAmphours = (float)((uint16_t)(data[5] | (data[6] << 8))) / 10.0f;

					// Extract depth of discharge (1 byte, scaled by 2)
					batteryInfo.packDOD      = (float)(data[7]) / 2.0f;
				}

				osMutexRelease(BatteryInfoMutexHandle);
			}
        }

			// Check if this message contains temperature data
			else if (batteryMsg.extendedID == TEMP_INFO_ID)
			{
	        	osStatus_t Batteryinfo_a5 = osMutexAcquire(BatteryInfoMutexHandle, UPDATING_MUTEX_TIMEOUT );
	        	if(Batteryinfo_a5 == osOK)
	        	{
	        		temp_info_counter++;
					// Ensure message has enough bytes
					if (batteryMsg.DLC >= 5)
					{
						// Highest temperature in pack
						batteryInfo.highTemp = data[0];

						// Lowest temperature (cast to signed value)
						batteryInfo.lowTemp  = (int8_t)data[2];

						// Average temperature
						batteryInfo.avgTemp  = data[4];
					}
					osMutexRelease(BatteryInfoMutexHandle);
				}
			}

			// Check if this message contains cell voltage data
			else if (batteryMsg.extendedID == CELL_VOLTAGES_ID)
			{
	        	osStatus_t Batteryinfo_a6 = osMutexAcquire(BatteryInfoMutexHandle, UPDATING_MUTEX_TIMEOUT );
	        	if(Batteryinfo_a6 == osOK)
	        	{
	        		cell_voltages_counter++;
					if (batteryMsg.DLC >= 6) // check also
					{
						// Lowest cell voltage (2 bytes, scaled by 10000)
						batteryInfo.lowCellVoltage    = (float)((uint16_t)(data[0] | (data[1] << 8))) / 10.0f; //was 10000 cuz i think volts, but if mV then.. :3

						// ID/index of lowest voltage cell
						batteryInfo.lowCellVoltageID  = data[2];

						// Highest cell voltage (2 bytes, scaled by 10000)
						batteryInfo.highCellVoltage   = (float)((uint16_t)(data[3] | (data[4] << 8))) / 10.0f;

						// ID/index of highest voltage cell
						batteryInfo.highCellVoltageID = data[5];
					}
				}
	        	osMutexRelease(BatteryInfoMutexHandle);
			}

        // Other message types (STARTUP_INFO, ERRORS, etc.) are not handled yet

    }


    else
    {
        // No message received → increment timeout counter
        missingOBMS_MsgCounter++; // this is bad
    }

    // If we missed too many messages in a row (timeout condition)
    if (((float) missingOBMS_MsgCounter/10.0) >= ORION_MSG_TIMEOUT_MS) // check
    {
    	osStatus_t MBMSStatus_a7 = osMutexAcquire(MBMSStatusMutexHandle, UPDATING_MUTEX_TIMEOUT );
    	osStatus_t MBMSTrip_a9 = osMutexAcquire(MBMSTripMutexHandle, UPDATING_MUTEX_TIMEOUT );
    	if(MBMSStatus_a7 == osOK && MBMSTrip_a9 == osOK)
    	{
			// Mark CAN communication as lost
			mbmsStatus.OBMS_CAN_RR = 0;
			// Trigger a fault/trip due to message timeout
			mbmsHardTrips.OBMS_msg_timeout_trip = 1;

			osMutexRelease(MBMSStatusMutexHandle);
			osMutexRelease(MBMSTripMutexHandle);
    	}
    	 else {
			if (MBMSStatus_a7 == osOK) {
				osMutexRelease(MBMSStatusMutexHandle);
			}
			if (MBMSTrip_a9 == osOK) {
				osMutexRelease(MBMSTripMutexHandle);
			}
    	 }

    }
}















