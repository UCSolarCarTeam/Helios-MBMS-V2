

/* Include statements */
#include "app_freertos.h"
#include "../Inc/StartupTask.h"
#include "BatteryControlTask.h"
#include "MBMS.h"
#include "ReadGPIO.h"

#include "main.h"
#include "cmsis_os.h"
#include <stdbool.h>
#include <stdint.h>




/* =============================== CONFIG =============================== */

/* Soft timeout for waiting on ESD release (ms). If exceeded, shutdown + exit task. */
#define ESD_SOFT_LIMIT_MS   2000U

/* Polling delay used while waiting for MPS/ESD state changes (ms). */
#define POLL_DELAY_MS       10U

#define DBG 0

/* ============================ PERMISSIONS (NO FLAGS) ========================= */
/* Global permissions structure.
 * StartupTask sets these "allow_*" booleans; other tasks read them to decide
 * whether to close contactors / enable systems.
 */

extern Permissions mbmsPermissions;
extern Contactor_Info contactorInfo[NUM_OF_CNTR];
extern MBMS_Status mbmsStatus;
extern Permissions mbmsPermissions;

extern uint32_t startup_Check_Counter;

static uint8_t startUpChecksComplete = 0;

/* ============================== HELPER FUNCTIONS ============================ */

/* Convert milliseconds to RTOS ticks.
 * - Uses osKernelGetTickFreq() to compute ticks/second
 * - Uses 64-bit math to avoid overflow
 * - Rounds up by adding 999 before dividing by 1000
 */
static uint32_t ms_to_ticks(uint32_t ms)
{
    uint64_t ticks = (uint64_t)ms * (uint64_t)osKernelGetTickFreq();
    return (uint32_t)((ticks + 999ULL) / 1000ULL);
}



/* ============================ PERMISSION HELPERS ============================ */
/* No RTOS event flags here: CHANGE #3
 * - These helpers simply set global booleans in gContactorPerms.
 * - Other tasks should read gContactorPerms.allow_* and act accordingly.
 */

// no control over closing/opening common
//static void allow_common_contactor(void)
//{
//    mbmsPermissions.common = 1;  /* permission: common contactor may close */
//}

static void give_lv_perms(void)
{
	osStatus_t Permissions_a1 = osMutexAcquire(PermissionsMutexHandle, UPDATING_MUTEX_TIMEOUT );
	if(Permissions_a1 == osOK)
	{
		mbmsPermissions.lv = 1;      /* permission: LV contactor may close */
	}
	osMutexRelease(PermissionsMutexHandle);
}

static void give_motor1_perms(void)
{
	osStatus_t Permissions_a2 = osMutexAcquire(PermissionsMutexHandle, UPDATING_MUTEX_TIMEOUT );
	if(Permissions_a2 == osOK)
	{
		mbmsPermissions.motor1 = 1;   /* permission: motor contactor(s) may close */
	}
	osMutexRelease(PermissionsMutexHandle);
}

static void give_motor2_perms(void)
{
	osStatus_t Permissions_a2 = osMutexAcquire(PermissionsMutexHandle, UPDATING_MUTEX_TIMEOUT );
	if(Permissions_a2 == osOK)
	{
		mbmsPermissions.motor2 = 1;   /* permission: motor contactor(s) may close */
	}
	osMutexRelease(PermissionsMutexHandle);
}



static void give_array_perms(void)
{
	osStatus_t Permissions_a3 = osMutexAcquire(PermissionsMutexHandle, UPDATING_MUTEX_TIMEOUT );
	if(Permissions_a3 == osOK)
	{
		mbmsPermissions.array = 1;   /* permission: array contactor may close */
	}
	osMutexRelease(PermissionsMutexHandle);
}





/* ============================ MAIN STARTUP FLOW ============================= */
/* Implements the startup sequence as a single linear flow.
 * Updates mbmsStatus.startupState at each major step for observability.
 */
static void Startup_Flowchart(void)
{
//	osStatus_t MBMSStatus_a1 = osMutexAcquire(MBMSStatusMutexHandle, READING_MUTEX_TIMEOUT );
//		if(MBMSStatus_a1 == osOK)
//		{
			/* Startup begins: explicit state so other tasks/logs know startup was entered */
			mbmsStatus.Startup_state = STARTUP_START;

			/* -------------------------------------------------------------------------
			 * Wait until MPS is ON
			 * - NO timeout (MPS is a user/system readiness condition, not a battery fault)
			 * - Polls every POLL_DELAY_MS to avoid busy-waiting
			 * ------------------------------------------------------------------------- */
			mbmsStatus.Startup_state = STARTUP_MPS_OPEN; /* Change #1 */

//#if DBG
			while (read_MPS() != MPS_ACTIVE)  /* Change #4*/
			{
				osDelay(POLL_DELAY_MS);   /* yield CPU while waiting */
				/* no timeout, no shutdown */
				if (mbmsStatus.System_state == BPS_FAULT)
				{
					osMutexRelease(MBMSStatusMutexHandle);
					osThreadTerminate(StartupTaskHandle);

				}
			}

			/* MPS is now closed */
			mbmsStatus.Startup_state = STARTUP_MPS_CLOSED;

			/* -------------------------------------------------------------------------
			 * Wait until ESD is NOT pressed (soft timeout)
			 * - If ESD remains pressed longer than ESD_SOFT_LIMIT_MS:
			 *   -> call shutdown handler and exit the thread
			 * ------------------------------------------------------------------------- */
			mbmsStatus.Startup_state = STARTUP_ESD_WAITING;


			while (read_ESD() == ESD_ACTIVE) // if esd is pressed
			{
				osDelay(POLL_DELAY_MS);  /* wait between polls */


				if (mbmsStatus.System_state == BPS_FAULT)
				{
					osMutexRelease(MBMSStatusMutexHandle);
					osThreadTerminate(StartupTaskHandle);

				}
			}


			/* ESD is released */
			mbmsStatus.Startup_state = STARTUP_ESD_RELEASED;

			// wait for main and common contactor to be closed
			while (read_Common_CNTR_Aux() != MAIN_CNTR_AUX_ACTIVE || read_Main_CNTR_Aux() != MAIN_CNTR_AUX_ACTIVE)
			{
				osDelay(POLL_DELAY_MS);
			}

			mbmsStatus.Startup_state = MAIN_COMMON_CLOSED;

			// delays this task, so BCT can run startup checks more times
			while (startup_Check_Counter < 5)
			{
				osDelay(POLL_DELAY_MS);
			}

			/* Checks completed successfully */
			mbmsStatus.Startup_state = STARTUP_CHECKS_COMPLETED;

			/* Permissions: LV contactor */
			give_lv_perms();
			mbmsStatus.Startup_state = STARTUP_LV_ENABLED;

			/* -------------------------------------------------------------------------
			 * Enable DCDC1 (Aux -> Main battery switching)
			 * - Then updates state to record DCDC1 enabled
			 * ------------------------------------------------------------------------- */
    		HAL_GPIO_WritePin(DCDC1_EN_GPIO_Port, DCDC1_EN_Pin, DCDC1_EN_ACTIVE);

    		mbmsStatus.Startup_state = STARTUP_DCDC1_ON;

    		/* -------------------------------------------------------------------------
    		 * Enable 12V CAN / front-of-car system power
    		 * - Updates state to record CAN12V enable step
     		* ------------------------------------------------------------------------- */
//#endif
    		// we are precharging here btw !!!!!
   			 HAL_GPIO_WritePin(_12V_CAN_PCHG_GPIO_Port, _12V_CAN_PCHG_Pin, _12V_CAN_PCHG_ACTIVE);
    			while (read_12V_CAN_State() != _12V_CAN_STATE_ACTIVE) {
   			 	osDelay(POLL_DELAY_MS);
   			 }
   			 HAL_GPIO_WritePin(_12V_CAN_EN_GPIO_Port, _12V_CAN_EN_Pin, _12V_CAN_EN_ACTIVE);
   			 HAL_GPIO_WritePin(_12V_CAN_PCHG_GPIO_Port, _12V_CAN_PCHG_Pin, !(_12V_CAN_PCHG_ACTIVE));

   			 mbmsStatus.Startup_state = STARTUP_12V_CAN_ON;  /* state added for visibility */

   			 /* Permissions: Motor contactor */
   			 give_motor1_perms();
   			 mbmsStatus.Startup_state = STARTUP_MOTOR1_ENABLED;
   			 give_motor2_perms();
   			 mbmsStatus.Startup_state = STARTUP_MOTOR2_ENABLED;

   			 /* -------------------------------------------------------------------------
   			  * Permissions: Array contactor
   			  * - NOTE: confirm whether "array contactor" implies charging vs just solar connect
   			  * -Jenny: array is just the solar array, but we have to be allowed to charge to turn them on
   			  * - Updates state to record array permission step
   			  * ------------------------------------------------------------------------- */
   			 /* NOTE: confirm with team if "array contactor" implies charging or just solar connect */
   			 give_array_perms();
   			 mbmsStatus.Startup_state = STARTUP_ARRAY_ENABLED;

   			 /* Startup flow complete */
   			 mbmsStatus.Startup_state = STARTUP_DONE;

   			 /* Exit thread cleanly (CMSIS-RTOS2) */
   			 osThreadTerminate(StartupTaskHandle);
			}

//		osMutexRelease(MBMSStatusMutexHandle);
//}




/* =============================== TASK ENTRY ================================= */
/* FreeRTOS/CMSIS task entry point.
 * Runs the startup flow and exits the thread when complete.
 */
void StartupTask(void *argument)
{
    (void)argument;      /* argument not used */

	enter_BOOT();
	while(1) {
	    Startup_Flowchart(); /* run the full startup sequence */
	}

    /* Safety fallback in case Startup_Flowchart ever returns */
    //osThreadTerminate(StartupTaskHandle);
}
