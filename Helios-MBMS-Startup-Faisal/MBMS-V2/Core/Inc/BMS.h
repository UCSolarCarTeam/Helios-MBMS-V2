#ifndef BMS_H_
#define BMS_H_

#include <stdint.h>



#ifndef BMS_H_
#define BMS_H_

#include <stdint.h>

/* ===================== CCP Heartbeat Config ===================== */

#ifndef CCP_HEARTBEAT_TIMEOUT_MS
#define CCP_HEARTBEAT_TIMEOUT_MS 300U
#endif

#ifndef CCP_COUNTER_MAX_SKIP
#define CCP_COUNTER_MAX_SKIP 1U
#endif

#ifndef CCP_CRC8_POLY
#define CCP_CRC8_POLY 0x07U
#endif

#ifndef CCP_CRC8_INIT
#define CCP_CRC8_INIT 0x00U
#endif

#ifndef CCP_CRC_INCLUDE_CAN_ID
#define CCP_CRC_INCLUDE_CAN_ID 1U
#endif

/* ... rest of your BMS types ... */

#endif /* BMS_H_ */






typedef struct
{
    uint8_t low_voltage;
    uint8_t motor;
    uint8_t array;
    uint8_t charge;
} contactor_CMND_t;

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

}MBMS_Status;




typedef struct
{
	uint8_t DCDC1_en;
	uint8_t MBMS_charge_12V_enable;
	uint8_t DCDC1_fault;
	uint8_t critical_fault;
	uint8_t charger_fault;
	uint8_t critical_OC;
}DCDC_Stack;




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
}MBMS_Hard_Trips;



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
}MBMS_Soft_Trips ;

typedef struct {
	uint8_t pre_charge_open;
	uint8_t pre_charge_close;
	uint8_t pre_charge_error;
	uint8_t pre_charge_open_or_close;
	uint8_t contactor_closing;
	uint8_t contactor_error;
	uint8_t contactor_opening_error;
	uint16_t line_current; // this is uint16 not Uint12, change CAN communication sheet!
	uint16_t charge_current;

}LV_board;

typedef struct {
	uint8_t pre_charge_open;
	uint8_t pre_charge_close;
	uint8_t pre_charge_error;
	uint8_t pre_charge_open_or_close;
	uint8_t contactor_closing;
	uint8_t contactor_error;
	uint8_t contactor_opening_error;
	uint16_t line_current; // this is uint16 not Uint12, change CAN communication sheet!
	uint16_t charge_current;

}Charge_board;


typedef struct {
	uint8_t pre_charge_open;
	uint8_t pre_charge_close;
	uint8_t pre_charge_error;
	uint8_t pre_charge_open_or_close;
	uint8_t contactor_closing;
	uint8_t contactor_error;
	uint8_t contactor_opening_error;
	uint16_t line_current; // this is uint16 not Uint12, change CAN communication sheet!
	uint16_t charge_current;

}Array_board;

typedef struct {
	uint8_t pre_charge_open;
	uint8_t pre_charge_close;
	uint8_t pre_charge_error;
	uint8_t pre_charge_open_or_close;
	uint8_t contactor_closing;
	uint8_t contactor_error;
	uint8_t contactor_opening_error;
	uint16_t line_current; // this is uint16 not Uint12, change CAN communication sheet!
	uint16_t charge_current;

}Motor_board;


// list [instnce]
// list [

Board_t* board_list[] = {
    &LV_board,
    &Motor_board,
    &Array_board,
    &Charge_board
};

uint64_t* heartbeat_list[] = {
    LV_heartbeat,
    Motor_heartbeat,
    Array_heartbeat,
    Charge_heartbeat
};


const int board_list_len = (int)(sizeof(board_list) / sizeof(board_list[0]));



#endif /* BMS_H_ */
