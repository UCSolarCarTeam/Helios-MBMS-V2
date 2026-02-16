#ifndef CAN_H_
#define CAN_H_

#include <stdint.h>

#define CAN_MAX_DATA_LEN 8u // full CAN FD payload size in bytes
//ORION EXTENDED IDS
#define BMU_HEARTBEAT 		0x300
#define STARTUP_INFO 		0x301
#define PACK_INFO 			0x302
#define ERRORS 				0x303
#define TEMP_INFO 			0x304
#define CELL_VOLTAGES 		0x305
#define MIN_MAX_VOLTAGES 	0x30A

//Contactors
#define CONTACTOR_MASK		0x1fffffe0
#define CONTACTOR_ID		0x210
#define CONTACTOR_HEARTBEAT	0x200

//some more defines idk what these do
#define MBMS_HEARTBEAT_ID 0x100
#define CONTACTOR_COMMAND_ID 0x101
#define MBMS_STATUS_ID 0x102
#define POWER_SELECTION_STATUS_ID 0x103
#define MBMS_TRIP_ID 0x104
#define MBMS_SOFT_BATTERY_LIMIT_WARNING_ID 0x105


typedef struct {
		uint16_t ID; //11 bit CAN ID. lower ID means higher priority
		uint32_t extendedID; // 29 bit extended ID
		uint32_t  DLC; // data length code: encoded value (0–64 for CAN FD)
		uint8_t  data[CAN_MAX_DATA_LEN]; // payload bytes
} CANmsg;




#endif // CAN_H_
