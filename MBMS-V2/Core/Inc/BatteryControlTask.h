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

#define HARD_MAX_CELL_VOLTAGE 4.10f //4.20V // send as 42
#define HARD_MIN_CELL_VOLTAGE 2.60f // 2.50V
#define HARD_MAX_TEMP 45
#define HARD_MIN_TEMP 2

/* Current? */
#define HARD_MAX_COMMON_CONTACTOR_CURRENT 	61.2f
#define HARD_MAX_MOTORS_CONTACTOR_CURRENT 	61.2f // tenths of an Amp 61.25 amps
#define HARD_MAX_ARRAY_CONTACTOR_CURRENT 	61.2f
#define HARD_MAX_LV_CONTACTOR_CURRENT 		61.2f
#define HARD_MAX_CHARGE_CONTACTOR_CURRENT  	61.2f
#define NO_CURRENT_THRESHOLD 				3 // (tenths of an AMPS). So if less than this, consider no current, if more than this, consider there is current

#define SOFT_MAX_CELL_VOLTAGE 3.9F
#define SOFT_MIN_CELL_VOLTAGE 3.5F
#define SOFT_MAX_TEMP 40
#define SOFT_MIN_TEMP 5
#define SOFT_MAX_COMMON_CONTACTOR_CURRENT 50.0f //was 290
#define SOFT_MAX_MOTORS_CONTACTOR_CURRENT 50.0f
#define SOFT_MAX_ARRAY_CONTACTOR_CURRENT 50.0f
#define SOFT_MAX_LV_CONTACTOR_CURRENT 50.0f
#define SOFT_MAX_CHARGE_CONTACTOR_CURRENT 50.0f
// ASK MILLAINE WHY THERES NO SOFT_MAX_PACK_VOLTAGE

#define MINIMUM_ORION_MESSAGE_RECEIVED 5

// delete below, i made active states in read gpio header
//#define MPS_ACTIVE_LEVEL 1 // MPS CLOSED - have power
//#define ESD_ACTIVE_LEVEL 1 // ESD PRESSED - BPS
//#define DCDC1_ENABLE_LEVEL 1 // DCDC1 is enabled

/* E.g. Temp Info is sent at 5Hz from OBMS which means one msg every 0.2 seconds
 * BCT runs one cycle every 10 ticks, (1 tick every millisecond), so every 10 ms
 * UpdateBatteryInfo gets things from queue with a timeout of 0
 * Thus is there are no messages, it increments the no messages counter by 1 every 10ms
 * Divide this counter by 10, and that is how many ms has passed since mbms recieved a msg from OBMS
 * Then take that number and compare it to ORION_MSG_TIMEOUT_MS
 * ORION_MSG_TIMEOUT_MS is currently set to 500ms meaning 0.5 seconds
 * We are saying if we receive no messages from OBMS within 0.5 seconds, smth is wrong
 * This makes sense bc we should AT LEAST be receiving a message every 0.2 seconds (Temp Info)
 *
 */
#define ORION_MSG_TIMEOUT_MS 500


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
void check_charging();

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
