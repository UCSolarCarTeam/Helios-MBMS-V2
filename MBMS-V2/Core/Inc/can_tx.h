#ifndef CAN_TX_
#define CAN_TX_

#include "CAN.h"
#include "stm32h5xx_hal_fdcan.h"


void CAN_Tx_Init(void);

// Send a single CAN/CAN FD frame described by msg.
// Returns HAL_OK on success, or an error code from the HAL.
HAL_StatusTypeDef CAN_Tx_Send(const CANmsg *msg);

#endif // CAN_TX_
