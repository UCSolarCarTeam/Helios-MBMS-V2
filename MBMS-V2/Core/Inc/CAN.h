#ifndef CAN_H_
#define CAN_H_

#include <stdint.h>

#define CAN_MAX_DATA_LEN 64u // full CAN FD payload size in bytes

typedef struct {
		uint16_t ID; //11 bit CAN ID. lower ID means higher priority
		uint32_t extendedID; // 29 bit extended ID
		uint8_t  DLC; // data length code: number of data bytes (0–64 for CAN FD)
		uint8_t  data[CAN_MAX_DATA_LEN]; // payload bytes
} CANmsg;

#endif // CAN_H_
