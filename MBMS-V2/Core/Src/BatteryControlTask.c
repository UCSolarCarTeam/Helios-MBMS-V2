/*
 * BatteryControlTask.c
 *
 *  Created on: Jan 17, 2026
 *      Author: m
 */


#include "BatteryControlTask.h"
#include <stdint.h>
#include "cmsis_os.h"
//#include "CANdefines.h"
//#include "StartupTask.h"
//#include "ShutoffTask.h"
//#include "ReadPowerGPIO.h"
//#include "CANMessageSenderTask.h"
//#include "MBMS.h"
//#include "main.h"


void BatteryControlTask(void* arg)
{
	uint32_t taskTickLastStart = osKernelGetTickCount();

    while(1)
    {
    	BCT_start_tick = osKernelGetTickCount();
    	BatteryControl();
    	BCT_end_tick = osKernelGetTickCount();
    	BCT_difference_tick = BCT_end_tick - BCT_start_tick;
    	BCT_difference_seconds = taskTickLastStart;

		taskTickLastStart += 10;
		osDelayUntil(taskTickLastStart);
    }
}
