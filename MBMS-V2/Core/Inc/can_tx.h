#ifndef CAN_TX_
#define CAN_TX_

#include "main.h"
#include "CAN.h"


void CAN_Tx_Init(void); //CAN TX setup (starts FD CAN)

// Send a single CAN/CAN FD frame described by msg.
// Returns HAL_OK on success, or an error code from the HAL.

HAL_StatusTypeDef CAN_Tx_Send(const CANmsg *msg);

void CAN_Tx_Task(void *argument);

#endif // CAN_TX_
