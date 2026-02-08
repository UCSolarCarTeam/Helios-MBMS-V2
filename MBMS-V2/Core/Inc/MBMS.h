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



#endif /* INC_MBMS_H_ */

