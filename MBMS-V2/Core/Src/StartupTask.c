

/* Include statements */
#include "app_freertos.h"
#include "../Inc/StartupTask.h"
#include "BatteryControlTask.h"
#include "MBMS.h"

#include "main.h"
#include "cmsis_os.h"
#include <stdbool.h>
#include <stdint.h>
#include "app_freertos.h"



/* =============================== CONFIG =============================== */

/* Soft timeout for waiting on ESD release (ms). If exceeded, shutdown + exit task. */
#define ESD_SOFT_LIMIT_MS   2000U

/* Polling delay used while waiting for MPS/ESD state changes (ms). */
#define POLL_DELAY_MS       10U

extern uint8_t startupCheck_Counter = 0;

/* ============================ PERMISSIONS (NO FLAGS) ========================= */
/* Global permissions structure.
 * StartupTask sets these "allow_*" booleans; other tasks read them to decide
 * whether to close contactors / enable systems.
 */

extern Permissions mbmsPermissions;
extern Contactor_Info contactorInfo[NUM_OF_CNTR];
extern MBMS_Status mbmsStatus;
extern Permissions mbmsPermissions;




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

uint8_t read_Main_Contactor() {
	return HAL_GPIO_ReadPin(Main_CNTR_Aux_GPIO_Port, Main_CNTR_Aux_Pin);
}

uint8_t read_Common_Contactor() {
	return HAL_GPIO_ReadPin(Common_CNTR_Aux_GPIO_Port, Common_CNTR_Aux_Pin);
}
/* Read whether the Manual Power Switch (MPS) is ON.
 * - If the GPIO symbols exist, read the pin and compare to active level
 * - If not defined yet, return true (so development/testing can proceed)
 */
static bool is_mps_on(void)
{
#if defined(MPS_GPIO_Port) && defined(MPS_Pin)
    return (HAL_GPIO_ReadPin(MPS_GPIO_Port, MPS_Pin) == MPS_ACTIVE_LEVEL); // Define "MPS_ACTIVE_LEVEL" //
#else
    return false;
#endif
}

/* Read whether the Emergency Stop (ESD) is pressed.
 * - If the GPIO symbols exist, read the pin and compare to active level
 * - If not defined yet, return false (assume not pressed)
 */
static bool is_esd_pressed(void)
{
#if defined(ESD_GPIO_Port) && defined(ESD_Pin)
    return (HAL_GPIO_ReadPin(ESD_GPIO_Port, ESD_Pin) == ESD_ACTIVE_LEVEL);
#else
    return true;
#endif
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
	mbmsPermissions.lv = 1;      /* permission: LV contactor may close */
}

static void give_motor_perms(void)
{
    mbmsPermissions.motor = 1;   /* permission: motor contactor(s) may close */
}

static void give_array_perms(void)
{
    mbmsPermissions.array = 1;   /* permission: array contactor may close */
}





/* ============================ MAIN STARTUP FLOW ============================= */
/* Implements the startup sequence as a single linear flow.
 * Updates mbmsStatus.startupState at each major step for observability.
 */
static void Startup_Flowchart(void)
{
    /* Startup begins: explicit state so other tasks/logs know startup was entered */
    mbmsStatus.Startup_state = STARTUP_START;

    /* -------------------------------------------------------------------------
     * Wait until MPS is ON
     * - NO timeout (MPS is a user/system readiness condition, not a battery fault)
     * - Polls every POLL_DELAY_MS to avoid busy-waiting
     * ------------------------------------------------------------------------- */
    mbmsStatus.Startup_state = STARTUP_MPS_OPEN; /* Change #1 */

    while (!is_mps_on())  /* Change #4*/
    {
        osDelay(POLL_DELAY_MS);   /* yield CPU while waiting */
        /* no timeout, no shutdown */
        if (mbmsStatus.System_state == BPS_FAULT) {
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


	while (is_esd_pressed()) // if esd is pressed
	{
		osDelay(POLL_DELAY_MS);  /* wait between polls */


		if (mbmsStatus.System_state == BPS_FAULT) {
			osThreadTerminate(StartupTaskHandle);

		}
	}


    /* ESD is released */
    mbmsStatus.Startup_state = STARTUP_ESD_RELEASED;

    // wait for main and common contactor to be closed
    while (read_Common_Contactor() != MAIN_CONTACTOR_ON_ACTIVE || read_Main_Contactor() != COMMON_CONTACTOR_ON_ACTIVE) {
    	osDelay(POLL_DELAY_MS);
    }

    mbmsStatus.Startup_state = MAIN_COMMON_CLOSED;



    // delays this task, so BCT can run startup checks more times
    while (startupCheck_Counter < 5) {
    	osDelay(POLL_DELAY_MS);

    }


    /* Checks completed successfully */
    mbmsStatus.Startup_state = STARTUP_CHECKS_COMPLETED;



    /* Permissions: LV contactor */
    give_lv_perms();
    mbmsStatus.Startup_state = STARTUP_LV_CLOSED;

    /* -------------------------------------------------------------------------
     * Enable DCDC1 (Aux -> Main battery switching)
     * - Only compiles if GPIO macros exist
     * - Then updates state to record DCDC1 enabled
     * ------------------------------------------------------------------------- */
#if defined(DCDC1_EN_GPIO_Port) && defined(DCDC1_EN_Pin) /* Change #2*/
    HAL_GPIO_WritePin(DCDC1_EN_GPIO_Port, DCDC1_EN_Pin, DCDC1_ENABLE_LEVEL);
#endif
    mbmsStatus.Startup_state = STARTUP_DCDC1_ON;

    /* -------------------------------------------------------------------------
     * Enable 12V CAN / front-of-car system power
     * - Only compiles if GPIO macros exist
     * - Updates state to record CAN12V enable step
     * ------------------------------------------------------------------------- */
#if defined(CAN12V_EN_GPIO_Port) && defined(CAN12V_EN_Pin)
    HAL_GPIO_WritePin(CAN12V_EN_GPIO_Port, CAN12V_EN_Pin, CAN12V_ENABLE_LEVEL);
#endif
    mbmsStatus.Startup_state = STARTUP_CAN12V_ON;  /* state added for visibility */

    /* Permissions: Motor contactor */
    give_motor_perms();
    mbmsStatus.Startup_state = STARTUP_MOTORS_ENABLED;

    /* -------------------------------------------------------------------------
     * Permissions: Array contactor
     * - NOTE: confirm whether "array contactor" implies charging vs just solar connect
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





/* =============================== TASK ENTRY ================================= */
/* FreeRTOS/CMSIS task entry point.
 * Runs the startup flow and exits the thread when complete.
 */
void StartupTask(void *argument)
{
    (void)argument;      /* argument not used */

    Startup_Flowchart(); /* run the full startup sequence */

    /* Safety fallback in case Startup_Flowchart ever returns */
    osThreadTerminate(StartupTaskHandle);
}
