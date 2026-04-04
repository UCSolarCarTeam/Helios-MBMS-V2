

/*
* MBMS.h
*
*  Created on: Jan 24, 2026
*      Author: m
*/

#ifndef INC_MBMS_H_
#define INC_MBMS_H_

/* Defines */
#define OPEN_CONTACTOR 0
#define CLOSE_CONTACTOR 1
#define CLOSING_CONTACTOR 1

/* Enums */
enum SecondaryContactor {
	LV,
	MOTOR,
	ARRAY,
	CHARGE
};


enum carStates {
	BOOT,
	STARTUP,
	FULLY_OPERATIONAL,
	CHARGING,
	BPS_FAULT,// High Trip
	MPS_DISCONNECTED,
	SOFT_TRIP // voltages, current, temp, is getting high but not dangerous
};

typedef struct {
	uint8_t lv;
	uint8_t motor;
	uint8_t array;
	uint8_t charge;
	uint8_t startupDone;
	uint8_t faulted;
} Permissions;


typedef struct {
	// pack info
    float packCurrent; // current can be -ve, 2-bytes
    float packVoltage; // 2-bytes
    float packSOC; // state of charge, 1-byte
    float packAmphours; // 2-bytes
    float packDOD; // Depth of Discharge, 1-byte
    // temperature info (each 1-byte)
    uint8_t highTemp;
    int8_t lowTemp;
    uint8_t avgTemp;
    // Cell voltages
    float lowCellVoltage;
    uint8_t lowCellVoltageID;
    float highCellVoltage;
    uint8_t highCellVoltageID;
    // min max voltage info (each 2-bytes)
//    uint16_t maxCellVoltage;
//    uint16_t minCellVoltage;
//    uint16_t maxPackVoltage;
//    uint16_t minPackVoltage;
} BatteryInfo;


#endif /* INC_MBMS_H_ */

