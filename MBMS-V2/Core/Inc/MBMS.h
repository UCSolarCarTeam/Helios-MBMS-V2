/*
 * MBMS.h
 *
 *  Created on: Feb 4, 2026
 *      Author: m
 */

#ifndef INC_MBMS_H_
#define INC_MBMS_H_

#include "main.h"

#define FREERTOS_TICK_PERIOD 1.0/configTICK_RATE_HZ


enum Contactor {
	COMMON = 0,
	MOTOR,
	ARRAY,
	LOWV,
	CHARGE
};

typedef struct {
	uint8_t motor;
	uint8_t array;
	uint8_t lv;
	uint8_t charge;
	uint8_t startupDone;
	uint8_t faulted;
} Permissions;

/*-------------------------------------*/
// TO DO: please create structs for the rest of the things u need (such as MBMS status, DCDC stack, etc. below)

typedef struct{
	uint8_t common;
	uint8_t motor;
	uint8_t array;
	uint8_t LV;
	uint8_t charge;
} ContactorCommand;

typedef struct {
	uint8_t nMainPowerSwitch;
	uint8_t ExternalShutdown;
	uint8_t EN1;
	uint8_t nDCDC_Fault;
	uint8_t n3A_OC;
	uint8_t nDCDC_On;
	uint8_t nCHG_Fault;
	uint8_t nCHG_On;
	uint8_t nCHG_LV_En;
	uint8_t ABATT_Disable;
	uint8_t Key;
} PowerSelectionStatus;


typedef struct {
	uint8_t prechargerClosed;
	uint8_t prechargerClosing;
	uint8_t prechargerError;
	uint8_t contactorClosed; //two bits become one variable, 00 = open, 01 = closed, 10 = closing
	uint8_t contactorClosing;
	uint8_t contactorError;
	int16_t lineCurrent;
	int16_t chargeCurrent;
	uint8_t contactorOpeningError;
	uint16_t heartbeat;
} ContactorInfo;


typedef struct {
	uint8_t highCellVoltageWarning;
	uint8_t lowCellVoltageWarning;
	uint8_t commonHighCurrentWarning;
	uint8_t motorHighCurrentWarning;
	uint8_t arrayHighCurrentWarning;
	uint8_t LVHighCurrentWarning;
	uint8_t chargeHighCurrentWarning;
	uint8_t highBatteryWarning;
	uint8_t highTemperatureWarning;
	uint8_t lowTemperatureWarning;

} MBMSSoftBatteryLimitWarning;


typedef struct {
	// pack info
    float packCurrent; // (common) current can be -ve, 2-bytes
    float packVoltage; // 2-bytes
    float packSOC; // State of Charge, 1-byte
    float packAmphours; // 2-bytes
    float packDOD; // Depth of Discharge, 1-byte

    // Temperature info (each 1-byte)
    uint8_t highTemp;
    int8_t lowTemp;
    uint8_t avgTemp;

    // Cell voltages
    float lowCellVoltage;
    uint8_t lowCellVoltageID;
    float highCellVoltage;
    uint8_t highCellVoltageID;

} BatteryInfo;

typedef struct{
	uint8_t auxilaryBattVoltage;
	uint8_t strobeBMSLight;
	uint8_t nChargeEnable;
	uint8_t nChargeSafety;
	uint8_t nDischargeEnable;
	uint8_t orionCANReceived;
	uint8_t dischargeShouldTrip;
	uint8_t chargeShouldTrip;
	uint8_t startupState;
	uint8_t carState;
}MBMSStatus;

typedef struct {
	uint8_t highCellVoltageTrip;
	uint8_t lowCellVoltageTrip;
	uint8_t commonHighCurrentTrip;
	uint8_t motorHighCurrentTrip;
	uint8_t arrayHighCurrentTrip;
	uint8_t LVHighCurrentTrip;
	uint8_t chargeHighCurrentTrip;
	uint8_t protectionTrip;
	uint8_t orionMessageTimeoutTrip;
	uint8_t contactorDisconnectedUnexpectedlyTrip;
	uint8_t contactorConnectedUnexpectedlyTrip;
	uint8_t highBatteryTrip;
	uint8_t commonHeartbeatDeadTrip;
	uint8_t motorHeartbeatDeadTrip;
	uint8_t arrayHeartbeatDeadTrip;
	uint8_t LVHeartbeatDeadTrip;
	uint8_t chargeHeartbeatDeadTrip;
	uint8_t MPSDisabledTrip;
	uint8_t ESDEnabledTrip;
	uint8_t highTemperatureTrip;
	uint8_t lowTemperatureTrip;

} MBMSTrip;


#endif /* INC_MBMS_H_ */

