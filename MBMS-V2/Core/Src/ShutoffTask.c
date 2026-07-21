/*
 * ShutoffTask.c
 *
 *  Created on: May 9, 2026
 *      Author: m
 */


#include <stdint.h>


#include "../Inc/ShutoffTask.h"
#include "StartupTask.h"
#include "CAN.h"
#include "cmsis_os.h"
#include "main.h"
#include "BatteryControlTask.h"
#include "ReadGPIO.h"
#include "MBMS.h"


extern Permissions mbmsPermissions;
extern Contactor_Info contactorInfo[NUM_OF_CNTR];


void ShutoffTask(void* arg)
{
    while(1)
    {
    	Shutoff();
    }
}


void Shutoff()
{

	uint32_t flags;

	while (1) {

		// wait for shutoff flag
		flags = osEventFlagsWait(shutoffFlagHandle, SHUTOFF_FLAG, osFlagsWaitAny | osFlagsNoClear, osWaitForever);

		//disable 12V CAN
		HAL_GPIO_WritePin(_12V_CAN_EN_GPIO_Port, _12V_CAN_EN_Pin, !_12V_CAN_EN_ACTIVE);

		// ensure charging is disabled
		HAL_GPIO_WritePin(_14V_Charge_EN_GPIO_Port, _14V_Charge_EN_Pin, !_14V_CHARGE_EN_ACTIVE);

		mbmsPermissions.motor1 = 0;
#if motor2def
		mbmsPermissions.motor2 = 0;
#endif
		mbmsPermissions.charge = 0;
		mbmsPermissions.array = 0;
		mbmsPermissions.lv = 0;




//
//		mbmsPermissions.motor1 = 0;
//		mbmsPermissions.motor2 = 0;
//		// wait to open CHECK THIS
//		while((contactorInfo[MOTOR1].contactor_close == CLOSE_CONTACTOR) ||
//			  (contactorInfo[MOTOR2].contactor_close == CLOSE_CONTACTOR))
//		{
//			osDelay(200);
//		}
//
//		mbmsPermissions.array = 0;
//		// wait to open CHECK THIS
//		while(contactorInfo[ARRAY].contactor_close == CLOSE_CONTACTOR) {
//			osDelay(200);
//		}
//
//		mbmsPermissions.lv = 0;
//		// wait to open CHECK THIS
//		while(contactorInfo[LV].contactor_close == CLOSE_CONTACTOR) {
//			osDelay(200);
//		}
//
//		mbmsPermissions.charge = 0;
//		// wait to open CHECK THIS
//		while(contactorInfo[CHARGE].contactor_close == CLOSE_CONTACTOR) {
//			osDelay(200);
//
//		}

		// Disable DCDC1
		HAL_GPIO_WritePin(DCDC1_EN_GPIO_Port, DCDC1_EN_Pin, !DCDC1_EN_ACTIVE);


		if((flags & HARD_BAT_LIMIT_FLAG) == HARD_BAT_LIMIT_FLAG) {
			while(1){
				osDelay(100);
				// wait for driver to turn off car using key
			}
		}

		else {
			while(read_MPS() != MPS_ACTIVE) {
				osDelay(200);
			}
			// start thread for startup!!!
			if(read_MPS() == MPS_ACTIVE) {
				enter_BOOT();
				osEventFlagsDelete(shutoffFlagHandle);
				shutoffFlagHandle = osEventFlagsNew(&shutoffFlag_attributes);
				osEventFlagsClear(shutoffFlagHandle, 0xffffffff);

				StartupTaskHandle = osThreadNew(StartupTask, NULL, &StartupTask_attributes);
			}

		}
	}

}




