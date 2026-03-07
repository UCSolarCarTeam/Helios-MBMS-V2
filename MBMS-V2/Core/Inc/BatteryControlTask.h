/*
 * BatteryControlTask.h
 *
 *  Created on: Jan 17, 2026
 *      Author: m
 */

#ifndef INC_BATTERYCONTROLTASK_H_
#define INC_BATTERYCONTROLTASK_H_

#include <stdint.h>
void BatteryControlTask(void* arg);
void BatteryControl();


void perms_init();
void MBMSStatus_init();

/* --------- "Private" Helper Functions --------- */

void checkKeyShutdown();
void updateContactorInfo(uint8_t contactor, uint8_t prechargerClosed, uint8_t prechargerClosing,
		uint8_t prechargerError, uint8_t contactorClosed, uint8_t contactorClosing,
		uint8_t contactorError, int16_t lineCurrent, int16_t chargeCurrent, uint8_t BPSerror);

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
void UpdateCounter(uint32_t * counter);




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





#endif /* INC_BATTERYCONTROLTASK_H_ */
