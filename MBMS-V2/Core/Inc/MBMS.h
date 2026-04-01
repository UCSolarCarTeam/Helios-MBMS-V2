/*
 * MBMS.h
 *
 *  Created on: Jan 24, 2026
 *      Author: m
 */

#ifndef INC_MBMS_H_
#define INC_MBMS_H_

#define UPDATING_MUTEX_TIMEOUT 5
#define READING_MUTEX_TIMEOUT 5

/* Defines */
#define OPEN_CONTACTOR 0
#define CLOSE_CONTACTOR 1
#define CLOSING_CONTACTOR 1

#define NUM_OF_CNTR 4
#define CONTACTOR_HEARTBEAT_TIMEOUT 1500 // milliseconds
#define MAX_HEARTBEAT_FAILS 3

/* Enums */
//typedef enum // match with the proper CAN ID's
//{
//    LV     = 0x100,
//    MOTOR  = 0x101,
//    ARRAY  = 0x102,
//    CHARGE = 0x103
//
//} ContactorCANID;

enum Contactors{
	LV = 0,
	MOTOR,
	ARRAY,
	CHARGE
};


enum carStates {
	BOOT,
	STARTUP,
	FULLY_OPERATIONAL,
	CHARGING,
	BPS_FAULT,
	MPS_DISCONNECTED,
	SOFT_TRIP
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


typedef struct
{
    uint8_t low_voltage;
    uint8_t motor;
    uint8_t array;
    uint8_t charge;
} Contactor_CMND_t;

typedef struct
{
	uint8_t Strobe_enable;
	uint8_t charge_safety;
	uint8_t discharge_enable;
	uint8_t charge_enable;
	uint8_t OBMS_CAN_RR;
	uint8_t MPS;
	uint8_t ESD;
	uint8_t Abatt_enable;
	uint8_t EVCC_connect;
	uint8_t Startup_state;
	uint8_t System_state;

} MBMS_Status;




typedef struct
{
	uint8_t DCDC1_en;
	uint8_t MBMS_charge_12V_enable;
	uint8_t DCDC1_fault;
	uint8_t critical_fault;
	uint8_t charger_fault;
	uint8_t critical_OC;
} DCDC_Stack;




typedef struct
{
	uint8_t High_volt_cell_trip;
	uint8_t Low_volt_cell_trip;
	uint8_t CMN_high_cur_trip;
	uint8_t LV_high_cur_trip;
	uint8_t MT_high_cur_trip;
	uint8_t AR_high_cur_trip;
	uint8_t CHG_high_cur_trip;
	uint8_t Resverse_cur_trip;
	uint8_t OBMS_msg_timeout_trip;
	uint8_t CNTR_disconnect_trip;
	uint8_t CNTR_connect_trip;
	uint8_t CMN_no_heartbeat_trip;
	uint8_t LV_no_heartbeat_trip;
	uint8_t MT_no_heartbeat_trip;
	uint8_t AR_no_heartbeat_trip;
	uint8_t CHG_no_heartbeat_trip;
	uint8_t ESD_trip;
	uint8_t High_temp_trip;
	uint8_t Low_temp_trip;

} MBMS_Hard_Trips;



typedef struct
{
	uint8_t High_volt_cell_Strip;
	uint8_t Low_volt_cell_Strip;
	uint8_t CMN_high_cur_Strip;
	uint8_t LV_high_cur_Strip;
	uint8_t MT_high_cur_Strip;
	uint8_t AR_high_cur_Strip;
	uint8_t CHG_high_cur_Strip;
	uint8_t High_temp_Strip;
	uint8_t Low_temp_Strip;

} MBMS_Soft_Trips ;


typedef struct {

	uint16_t heartbeat;
	uint8_t precharge_close; // pre-charge acts as a resistor to resist voltage spikes
	uint8_t precharge_closing;
	uint8_t precharge_error;
	uint8_t contactor_close;
	uint8_t contactor_closing;
	uint8_t contactor_error;
	uint8_t contactor_opening_error;
	uint16_t line_current; // this is uint16 not Uint12, change CAN communication sheet!
	uint16_t charge_current;
	uint32_t heartbeat_check_count;

} Contactor_Info;




#endif /* INC_MBMS_H_ */
