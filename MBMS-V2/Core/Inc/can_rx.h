/*
 * can_rx.h
 *
 *  Created on: Jan 4, 2026
 *      Author: helay
 */

#ifndef CAN_RX
#define CAN_RX

#include "CAN.h"


void CAN_Rx_Task(void *argument); //Free RTOS task that listens for CAN frames and will queue them

#endif /* CAN_RX*/
