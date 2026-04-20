#include "BatteryControlTask.h"
#include <stdint.h>
#include "cmsis_os.h"
#include "cmsis_os2.h"
#include "CAN.h"
//#include "StartupTask.h"
//#include "ShutoffTask.h"
#include "ReadGPIO.h"
//#include "CANMessageSenderTask.h"
#include "MBMS.h"
#include "main.h"
#include <string.h>
#include <stdbool.h>
#include <app_freertos.h>

uint32_t BCT_start_tick = 0;
uint32_t BCT_end_tick = 0;
uint32_t BCT_difference_tick = 0;
uint32_t BCT_difference_seconds = 0;
uint32_t BCT_Counter = 0;
uint32_t startup_Check_Counter = 0;
uint8_t carState = BOOT;

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


void MBMSStatus_init(void)
{
    memset(&mbmsStatus, 0, sizeof(mbmsStatus));
    mbmsStatus.Abatt_EN = 1;
}




void perms_init()
{
    // Reset all system permissions to safe defaults.
    // This prevents the battery from charging or discharging
    // until the startup enter_SOFT_TRIPs are complete.

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

    /* 6. Reset Counters */
    BCT_Counter = 0;

    missingOBMS_MsgCounter = 0;

    startup_Check_Counter = 0;

	pack_info_counter = 0;
	temp_info_counter = 0;
	cell_voltages_counter = 0;

	/* 7. Reset Heartbeats */
	for(int i = 0; i < NUM_OF_CNTR; i++) {
		previousHeartbeats[i] = 0;
		heartbeatLastUpdatedTime[i] = 0;
		contactorInfo[i].heartbeat = 0;
		/* 8. Make sure contactors open */
		if (!checkContactorsOpen() || !checkPrechargersOpen())
	{
		enter_BPS_FAULT();
		return;
	}
	}

}

void enter_MPS_DISCONNECTED()
{
	//mbmsHardTrips
	carState = MPS_DISCONNECTED;
	mbmsPermissions.faulted = 1; //contactors not allowed to close
	osEventFlagsSet(shutoffFlagHandle, (MPS_FLAG | SHUTOFF_FLAG)); //idk if I understnad this
}

void enter_BPS_FAULT()
{
	HAL_GPIO_WritePin(BPS_Fault_GPIO_Port, BPS_Fault_Pin, 1); // strobe enabling essentially
	mbmsPermissions.faulted = 1;
	carState = BPS_FAULT;

	osStatus_t a = osMutexAcquire(MBMSStatusMutexHandle, UPDATING_MUTEX_TIMEOUT);
	if(a == osOK) {
		// update mbms status
		mbmsStatus.BPS_Fault = 1;
		osMutexRelease(MBMSStatusMutexHandle);

	}
	osEventFlagsSet(shutoffFlagHandle, (HARD_BAT_LIMIT_FLAG | SHUTOFF_FLAG));
}

void enter_SOFT_TRIP()
{
	carState = SOFT_TRIP;
	mbmsPermissions.faulted = 1;
}

void enter_CHARGING()
{
	carState = CHARGING;
}

void enter_FULLY_OPERATIONAL()
{
	carState = FULLY_OPERATIONAL;
}



/*-------------------------------------------*/
/* Startup enter_SOFT_TRIPs */
void startupCheck() // change after this function is done: waitForFirstHeartbeats
{
    /* Run startup gate checks in order. If any fail, enter fault. */
    if (waitForFirstHeartbeats())
    {
        enter_BPS_FAULT();   // preferred name from your header
        return;
    }

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
uint8_t waitForFirstHeartbeats() {


	static uint8_t heartbeatFailCounter[NUM_OF_CNTR] = {0};
	uint8_t dead = 0;


	for(int i = 0; i < NUM_OF_CNTR; i++) {

		//heartbeat_check_count++;

		// case that a ccp heartbeat has died
		if (heartbeatFailCounter[i] > MAX_HEARTBEAT_FAILS) {
			// from enum
			switch (i) {
				case LV:
					mbmsHardTrips.LV_no_heartbeat_trip = 1;
					break;
				case MOTOR:
					mbmsHardTrips.MT_no_heartbeat_trip = 1;
					break;
				case ARRAY:
					mbmsHardTrips.AR_no_heartbeat_trip = 1;
					break;
				case CHARGE:
					mbmsHardTrips.CHG_no_heartbeat_trip = 1;
					break;
			}

			dead = 1;
			return dead;
		}

		// case that the heartbeat has reached max value
		if (previousHeartbeats[i] >= 65535) { // check this logic lol
			previousHeartbeats[i] = 0;
		}

		// case that heartbeat update has timed out
		// This checks whether the heartbeat did not increase.
		if(previousHeartbeats[i] >= contactorInfo[i].heartbeat){
			// If the heartbeat hasn't changed for too long, the system assumes it may have stalled.
			if(((osKernelGetTickCount() - heartbeatLastUpdatedTime[i])) > CONTACTOR_HEARTBEAT_TIMEOUT) { // where contactor_heartbeat_timeout is how often a heartbeat is sent out/recieved
				// The failure counter increases.
				heartbeatFailCounter[i]++;

			}
		}
		// If the heartbeat did increase, the controller is alive. (Updates the last heartbeat time, Resets the failure counter, Stores the new heartbeat value)
		else {
			heartbeatLastUpdatedTime[i] = osKernelGetTickCount();
			heartbeatFailCounter[i] = 0;
			previousHeartbeats[i] = (contactorInfo[i].heartbeat);
		}


	}

	return dead;

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
	uint8_t pass = 1;
    for (int i = 0; i < NUM_OF_CNTR; i++)
    {
        /* If the precharger is reported closed, then it is NOT open. */
        if (contactorInfo[i].precharge_close == CLOSE_CONTACTOR)
        {
            pass = 0;
            return pass;
        }
    }
    return pass;
}





uint8_t checkContactorsOpen()
{
	uint8_t pass = 1;
    for (int i = 0; i < NUM_OF_CNTR; i++)
    {
        if (contactorInfo[i].contactor_close == CLOSE_CONTACTOR)
        {
        	pass = 0;
            return pass;
        }
    }
    return pass;
}


/* ----------------------- */

/* ------ Main Control Functions ----- */

void SystemStateMachine()
{
	// Make  is plugged in to stand in for the CAN msg
	uint8_t plugged = read_EVCC_12_SW() == EVCC_12_SW_ACTIVE;

	switch (carState)
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
			mbmsHardTrips.ESD_trip = 1;
			enter_BPS_FAULT();
		}

		if(mbmsStatus.Startup_state == COMPLETED)
		{
			enter_FULLY_OPERATIONAL();
		}

		break;


	case FULLY_OPERATIONAL:

		if(read_MPS() != MPS_ACTIVE)
		{
			enter_MPS_DISCONNECTED();
					break;
		}

		if(plugged && (read_Charge_EN() == CHARGE_ENABLE_ACTIVE))
		{
			/*idk if i understand this */
			mbmsPermissions.lv = 0;
			mbmsPermissions.motor = 0;
			HAL_GPIO_WritePin(_12V_CAN_State_GPIO_Port, _12V_CAN_State_Pin, GPIO_PIN_RESET); //12V CAN Disabled
		}

		if(plugged && (contactorInfo[LV].contactor_close == OPEN_CONTACTOR) && (contactorInfo[MOTOR].contactor_close == OPEN_CONTACTOR))
		{
			HAL_GPIO_WritePin(_14V_Charge_EN_GPIO_Port, _14V_Charge_EN_Pin, CHARGE_ENABLE_ACTIVE); //ENABLE THE CHARGER
			mbmsPermissions.charge = 1;
		}

		if(plugged && (contactorInfo[CHARGE].contactor_close == CLOSE_CONTACTOR))
		{
			enter_CHARGING();
		}

		Check_ContactorHeartbeats();
		Update_SoftTripStruct();
		Update_TripStruct();

		break;

	case CHARGING:

		if(read_MPS() == MPS_ACTIVE)
		{
			enter_MPS_DISCONNECTED();
			break;
		}

		// if charger unplugged & allowed to discharge
		if( !plugged && (read_Discharge_EN() == DISCHARGE_ENABLE_ACTIVE))
		{
			HAL_GPIO_WritePin(_14V_Charge_EN_GPIO_Port, _14V_Charge_EN_Pin, !_14V_CHARGE_EN_ACTIVE); //Discharge THE CHARGER
			mbmsPermissions.charge = 0;
		}

		// once charge cntr is opened, close 12V CAN pchg
		if(contactorInfo[CHARGE].contactor_close == OPEN_CONTACTOR)
		{
			HAL_GPIO_WritePin(_12V_CAN_PCHG_GPIO_Port, _12V_CAN_PCHG_Pin, _12V_CAN_PCHG_ACTIVE);
		}

		// once done 12V CAN pchging, close 12V CAN cntr
		if ((read_12V_CAN_State() == _12V_CAN_STATE_ACTIVE) && (read_12V_CAN_PCHG() == _12V_CAN_PCHG_ACTIVE)) {
			HAL_GPIO_WritePin(_12V_CAN_EN_GPIO_Port, _12V_CAN_EN_Pin, _12V_CAN_EN_ACTIVE);
			HAL_GPIO_WritePin(_12V_CAN_PCHG_GPIO_Port, _12V_CAN_PCHG_Pin, !(_12V_CAN_PCHG_ACTIVE));
		}

		// once 12V CAN fully enabled, enable LV & motor
		if (read_12V_CAN_EN() == _12V_CAN_EN_ACTIVE) {
			mbmsPermissions.lv = 1;
			mbmsPermissions.motor = 1;
		}

		// finally, car becomes fully op
		if((contactorInfo[LV].contactor_close == CLOSE_CONTACTOR) && (contactorInfo[MOTOR].contactor_close == CLOSE_CONTACTOR))
		{
			enter_FULLY_OPERATIONAL();
		}

		Check_ContactorHeartbeats();
		Update_SoftTripStruct();
		Update_TripStruct();

		break;

	case BPS_FAULT:
		break;

	case MPS_DISCONNECTED:
		break;

	case SOFT_TRIP:

		if(batteryInfo.highCellVoltage == 1)
		{
			mbmsPermissions.charge = 0;
			mbmsPermissions.array  = 0;
		}
		if(batteryInfo.lowCellVoltage == 1)
		{
			mbmsPermissions.motor = 0;
		}
		if(read_MPS() != MPS_ACTIVE)
		{
			enter_MPS_DISCONNECTED();
			break;
		}

		Check_ContactorHeartbeats();
		Update_TripStruct();
		break;
	}
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
		if((mbmsPermissions.lv) && (contactorInfo[LV].contactor_close != CLOSE_CONTACTOR) && (mbmsStatus.discharge_enable == DISCHARGE_ENABLE_ACTIVE)){
			contactorCommand.LV = CLOSE_CONTACTOR;
		}
		else if ((mbmsPermissions.motor) && (contactorInfo[MOTOR].contactor_close != CLOSE_CONTACTOR ) && (mbmsStatus.discharge_enable == DISCHARGE_ENABLE_ACTIVE)){
			contactorCommand.motor = CLOSE_CONTACTOR;
		}
		else if ((mbmsPermissions.array) && (contactorInfo[ARRAY].contactor_close != CLOSE_CONTACTOR) && (mbmsStatus.charge_enable == CHARGE_ENABLE_ACTIVE)){
			contactorCommand.array = CLOSE_CONTACTOR;
		}
		else if ((mbmsPermissions.charge) && (contactorInfo[CHARGE].contactor_close != CLOSE_CONTACTOR) && (mbmsStatus.charge_enable == CHARGE_ENABLE_ACTIVE)){
			contactorCommand.charge = CLOSE_CONTACTOR;
		}
	}


	if (!mbmsPermissions.lv){
		contactorCommand.LV = OPEN_CONTACTOR;
	}

	if (!mbmsPermissions.motor){
		contactorCommand.motor = OPEN_CONTACTOR;
	}

	if (!mbmsPermissions.array){
		contactorCommand.array = OPEN_CONTACTOR;
	}

	if (!mbmsPermissions.charge){
		contactorCommand.charge = OPEN_CONTACTOR;
	}
}



/*-------------------------*/
/*----- Checking for Trips & Strips & Dead Heartbeats Functions -----*/


void Update_TripStruct()
{
	static uint8_t BPS_Fault = 0;

	osStatus_t acquire = osMutexAcquire(MBMSTripMutexHandle, UPDATING_MUTEX_TIMEOUT);
	if(acquire == osOK)
	{

		// Checking High Current & Reverse Current Trips
		osStatus_t a1 = osMutexAcquire(ContactorInfoMutexHandle, UPDATING_MUTEX_TIMEOUT );
		if(a1 == osOK)
		{
			if(batteryInfo.packCurrent > HARD_MAX_COMMON_CONTACTOR_CURRENT )
			{
				mbmsHardTrips.CMN_high_cur_trip = 1;
				BPS_Fault = 1;
			}

			if(contactorInfo[MOTOR].line_current > HARD_MAX_MOTORS_CONTACTOR_CURRENT)
			{
				mbmsHardTrips.MT_high_cur_trip = 1;
				BPS_Fault = 1;
			}

			if(contactorInfo[ARRAY].line_current > HARD_MAX_ARRAY_CONTACTOR_CURRENT)
			{
				mbmsHardTrips.AR_high_cur_trip = 1;
				BPS_Fault = 1;
			}

			if(contactorInfo[LV].line_current > HARD_MAX_LV_CONTACTOR_CURRENT)
			{
				mbmsHardTrips.LV_high_cur_trip = 1;
				BPS_Fault = 1;
			}

			if(contactorInfo[CHARGE].line_current > HARD_MAX_CHARGE_CONTACTOR_CURRENT)
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

			osMutexRelease(ContactorInfoMutexHandle);
		}


		// Checking Battery Related Trips!!
		osStatus_t a2 = osMutexAcquire(BatteryInfoMutexHandle, READING_MUTEX_TIMEOUT);
		if(a2 == osOK)
		{
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

			osMutexRelease(BatteryInfoMutexHandle);
		}

		// Checking Missing Orion Messages Trip
		osStatus_t a3 = osMutexAcquire(MBMSStatusMutexHandle, READING_MUTEX_TIMEOUT);
		if (a3 == osOK)
		{
			//if orion CAN msg wasnt received recently, trip set
			if(!(mbmsStatus.OBMS_CAN_RR))
			{
				mbmsHardTrips.OBMS_msg_timeout_trip = 1;
				BPS_Fault = 1;
			}
			osMutexRelease(MBMSStatusMutexHandle);
		}

		// Checking contactor disconnected & connected unexpectedly trip
		osStatus_t a4 = osMutexAcquire(ContactorCommandMutexHandle, READING_MUTEX_TIMEOUT);
		if (a4 == osOK)
		{
			/* Contactor disconnected unexpectedly */
			/* To check, we compare a minimum current draw with the state of the contactor */
			if(((contactorCommand.motor == CLOSE_CONTACTOR) 	 && 	(contactorInfo[MOTOR].line_current < NO_CURRENT_THRESHOLD)) 	 ||
			  ((contactorCommand.array == CLOSE_CONTACTOR) 		 && 	(contactorInfo[ARRAY].line_current < NO_CURRENT_THRESHOLD)) 	 ||
			  ((contactorCommand.LV == CLOSE_CONTACTOR) 		 && 	(contactorInfo[LV].line_current < NO_CURRENT_THRESHOLD)) 		 ||
			  ((contactorCommand.charge == CLOSE_CONTACTOR) 	 && 	(contactorInfo[CHARGE].line_current < NO_CURRENT_THRESHOLD))
			)
			{
				mbmsHardTrips.CNTR_disconnect_trip = 1;
				BPS_Fault = 1;

			}

			/* Contactor connected unexpectedly trip */
			if(((contactorCommand.motor == OPEN_CONTACTOR) 	&& 	(contactorInfo[MOTOR].line_current >= NO_CURRENT_THRESHOLD)) 	 ||
			  ((contactorCommand.array == OPEN_CONTACTOR) 	&& 	(contactorInfo[ARRAY].line_current >= NO_CURRENT_THRESHOLD)) 	 ||
			  ((contactorCommand.LV == OPEN_CONTACTOR) 		&& 	(contactorInfo[LV].line_current >= NO_CURRENT_THRESHOLD)) 		 ||
			  ((contactorCommand.charge == OPEN_CONTACTOR) 	&& 	(contactorInfo[CHARGE].line_current >= NO_CURRENT_THRESHOLD)))
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
		}

		// Check ESD
		if (read_ESD() == ESD_ACTIVE) {
			BPS_Fault = 1;
		}

		// Finally, if there were any trips, go to BPS FAULT state!!!!
		if(BPS_Fault)
		{
			enter_BPS_FAULT();
		}

	}
}

// FAISAL PLEASE IMPLEMENT YOUR FUNCTIONS HERE
void Update_SoftTripStruct() {

}

void Check_ContactorHeartbeats() {

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



void Update_DCDCStackStruct(void)
{

	dcdc_stack.DCDC1_en = read_DCDC1_EN();
	dcdc_stack._14V_Charge_EN = read_14V_Charge_EN();
	dcdc_stack.nDCDC_Fault = read_nDCDC_Fault();
	dcdc_stack._12V_Critical_Fault = read_12V_Critical_Fault();
	dcdc_stack._14V_Charger_Fault = read_14V_Charger_Fault();
	dcdc_stack._12V_Critical_UC = read_12V_Critical_UC();

}


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

        // Indicate CAN communication is healthy
        mbmsStatus.OBMS_CAN_RR = 1;

        // Pointer to the raw data bytes in the CAN message
        uint8_t *data = batteryMsg.data;

        // Check if this message contains pack-level information
        if (batteryMsg.extendedID == PACK_INFO_ID)
        {
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
        }

        // Check if this message contains temperature data
        else if (batteryMsg.extendedID == TEMP_INFO_ID)
        {
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
        }

        // Check if this message contains cell voltage data
        else if (batteryMsg.extendedID == CELL_VOLTAGES_ID)
        {
            // Ensure message has enough bytes
            if (batteryMsg.DLC >= 6) // check also
            {
                // Lowest cell voltage (2 bytes, scaled by 10000)
                batteryInfo.lowCellVoltage    = (float)((uint16_t)(data[0] | (data[1] << 8))) / 10000.0f;

                // ID/index of lowest voltage cell
                batteryInfo.lowCellVoltageID  = data[2];

                // Highest cell voltage (2 bytes, scaled by 10000)
                batteryInfo.highCellVoltage   = (float)((uint16_t)(data[3] | (data[4] << 8))) / 10000.0f;

                // ID/index of highest voltage cell
                batteryInfo.highCellVoltageID = data[5];
            }
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
        // Mark CAN communication as lost
        mbmsStatus.OBMS_CAN_RR = 0;

        // Trigger a fault/trip due to message timeout
        mbmsHardTrips.OBMS_msg_timeout_trip = 1;
    }
}















