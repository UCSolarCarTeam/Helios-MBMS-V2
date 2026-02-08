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

/* "private" helper functions */
void initiateBPSFault();
void checkKeyShutdown();
void updateContactorInfo(uint8_t contactor, uint8_t prechargerClosed, uint8_t prechargerClosing,
		uint8_t prechargerError, uint8_t contactorClosed, uint8_t contactorClosing,
		uint8_t contactorError, int16_t lineCurrent, int16_t chargeCurrent, uint8_t BPSerror);

void startupCheck();
uint8_t waitForFirstHeartbeats();
uint8_t startupBatteryCheck();
uint8_t checkPrechargersOpen();
uint8_t checkContactorsOpen();

void clear_Trips();
void clear_SoftTrips();



/* "public" functions */
void Update_ContactorInfoStruct();
void Update_BatteryInfoStruct();
void Update_DCDCStackStruct();

void Update_TripStruct();
void Update_SoftTripStruct();

void CheckContactorHeartbeats();

void SystemStateMachine();
void UpdateContactors();

void UpdateCounter(uint32_t * counter);

void enter_BOOT();





#endif /* INC_BATTERYCONTROLTASK_H_ */
