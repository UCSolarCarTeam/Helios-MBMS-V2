/*
 * BatteryControlTask.h
 *
 *  Created on: Jan 17, 2026
 *      Author: m
 */

#ifndef INC_BATTERYCONTROLTASK_H_
#define INC_BATTERYCONTROLTASK_H_

#include <stdint.h>

/* Battery Limit Defines */

#define HARD_MAX_CELL_VOLTAGE 4.20f //4.20V
#define HARD_MIN_CELL_VOLTAGE 2.50f // 2.50V
#define HARD_MAX_TEMP 60
#define HARD_MIN_TEMP 0

<<<<<<< HEAD
/* Current? */
#define HARD_MAX_COMMON_CONTACTOR_CURRENT 	300
#define HARD_MAX_MOTORS_CONTACTOR_CURRENT 	300
#define HARD_MAX_ARRAY_CONTACTOR_CURRENT 	300
#define HARD_MAX_LV_CONTACTOR_CURRENT 		300
#define HARD_MAX_CHARGE_CONTACTOR_CURRENT  	300
#define NO_CURRENT_THRESHOLD 				3 // (AMPS). So if less than this, consider no current, if more than this, consider there is current


#define MINIMUM_ORION_MESSAGE_RECEIVED 5
=======
#define MPS_ACTIVE_LEVEL 1 // MPS CLOSED - have power
#define ESD_ACTIVE_LEVEL 1 // ESD PRESSED - BPS
#define DCDC1_ENABLE_LEVEL 1 // DCDC1 is enabled
>>>>>>> 4bfb9aefd3d790d7e27f91310c70ece72f991738


void BatteryControlTask(void* arg);
void BatteryControl();


/* --------- "Private" Helper Functions --------- */

void checkKeyShutdown();
void updateContactorInfo(uint8_t contactor, uint8_t prechargerClosed, uint8_t prechargerClosing,
		uint8_t prechargerError, uint8_t contactorClosed, uint8_t contactorClosing,
		uint8_t contactorError, int16_t lineCurrent, int16_t chargeCurrent, uint8_t BPSerror);

// Private Init Functions
void perms_init();
void MBMSStatus_init();

// Private Startup Check Functions
void startupCheck();
uint8_t waitForFirstHeartbeats();
uint8_t startupBatteryCheck();
uint8_t checkPrechargersOpen();
uint8_t checkContactorsOpen();

// Private Switch State Functions
void enter_BOOT();

void enter_MPS_DISCONNECTED();

void enter_BPS_FAULT();

void enter_SOFT_TRIP();

void enter_CHARGING();

void enter_FULLY_OPERATIONAL();

// Private Reset Functions

void clear_Trips();
void clear_SoftTrips();

// Update BCT Counter Function
void update_Counter(uint32_t * counter);

/* -------------------------------------- */



/* --------- "Public" Functions --------- */

// Update Struct Functions (including checking trips/strips)
void Update_ContactorInfoStruct();
void Update_BatteryInfoStruct();
void Update_DCDCStackStruct();
void Update_TripStruct();
void Update_SoftTripStruct();


// Checking CCPS Alive Function
void Check_ContactorHeartbeats();

// Control Functions
void SystemStateMachine();
void Control_Contactors();

/* -------------------------------------- */



#endif /* INC_BATTERYCONTROLTASK_H_ */
