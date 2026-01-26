#include "../Inc/StartupTask.h"
#include "main.h"
#include "cmsis_os.h"

/* ============================================================================
 * StartupTask.c
 *
 * Purpose (matches the linear flowchart):
 *   1) Startup task begins
 *   2) Wait until MPS is ON (soft timeout)
 *   3) Wait until ESD is NOT pressed (soft timeout)
 *   4) Run startup checks (BCT)
 *   5) Give permission: Common contactor
 *   6) Give permission: LV contactor
 *   7) Enable DCDC1 (switch Aux -> Main battery)
 *   8) Enable 12V CAN
 *   9) Give permission: Motor contactor
 *  10) Give permission: Array contactor
 *  11) Update state = DONE
 *  12) Terminate startup task
 * ============================================================================
 */


/* ================================ TIMING CONFIG ============================= */

/* How long we wait for MPS to turn on before failing (soft limit) */
#define MPS_SOFT_LIMIT_MS   10000U

/* How long we wait for ESD to be released before failing (soft limit) */
#define ESD_SOFT_LIMIT_MS   2000U

/* How often we poll inputs while waiting */
#define POLL_DELAY_MS       10U


/* ================================ RTOS HANDLES ==============================
 * These are created by CubeMX in freertos.c.
 * - startupTaskHandle: handle for this task (used to terminate it)
 * - contactorPermissionsFlagHandle: event flags used to grant permissions
 */
extern osThreadId_t     startupTaskHandle;
extern osEventFlagsId_t contactorPermissionsFlagHandle;


/* =============================== GLOBAL STATUS ==============================
 * Global MBMS status instance.
 * Other tasks can read this to see where startup is in the flowchart.
 */
MBMS_Status_t mbmsStatus = {0};


/* ============================== HELPER FUNCTIONS ============================ */

/* Convert milliseconds into RTOS ticks (used for timeouts) */
static uint32_t ms_to_ticks(uint32_t ms)
{
    /* Multiply ms by tick frequency, then convert to seconds scale */
    return (ms * osKernelGetTickFreq()) / 1000U;
}


/* Read whether the Manual Power Switch (MPS) is ON */
static bool is_mps_on(void)
{
#if defined(MPS_GPIO_Port) && defined(MPS_Pin)
    /* Read the GPIO pin and compare to the active level for MPS */
    return (HAL_GPIO_ReadPin(MPS_GPIO_Port, MPS_Pin) == MPS_ACTIVE_LEVEL);
#else
    /* If the pin is not defined yet, assume ON so startup code can run */
    return true;
#endif
}


/* Read whether the Emergency Stop (ESD) is pressed */
static bool is_esd_pressed(void)
{
#if defined(ESD_GPIO_Port) && defined(ESD_Pin)
    /* Read the GPIO pin and compare to the active level for ESD */
    return (HAL_GPIO_ReadPin(ESD_GPIO_Port, ESD_Pin) == ESD_ACTIVE_LEVEL);
#else
    /* If the pin is not defined yet, assume NOT pressed */
    return false;
#endif
}


/* ============================ PERMISSION HELPERS ============================
 * These wrap RTOS event flags, so "permissions" are easier to manage.
 * In this design, StartupTask sets permission bits; other tasks respond to them.
 */

/* Clear a specific permission bit (unused for now, but kept for future use) */
static void __attribute__((unused)) perms_clear(uint32_t mask)
{
    /* Clear the event flag bits specified in mask */
    osEventFlagsClear(contactorPermissionsFlagHandle, mask);
}

/* Check if a permission bit is currently set (unused for now, but kept) */
static bool __attribute__((unused)) perms_is_set(uint32_t mask)
{
    /* Read current flags and check if the masked bits are set */
    return (osEventFlagsGet(contactorPermissionsFlagHandle) & mask) != 0U;
}


/* ============================= FAILURE HANDLING =============================
 * Flowchart fail states (left side).
 * Called when:
 *  - MPS timeout
 *  - ESD timeout
 *  - startup checks failed
 *
 * Future behavior should include things like:
 *  - open contactors / revoke permissions
 *  - set fault flags
 *  - log the failure reason
 */
static void startup_fail_shutdown(void)
{
    /* TODO: implement real shutdown behavior */
}


/* ============================== STARTUP CHECKS ==============================
 * The BatteryControlTask (BCT) will eventually implement real checks.
 * This is declared weak so BCT can override it later.
 */
__attribute__((weak)) bool BCT_AreStartupChecksOk(void)
{
    /* Fail-safe default: if nothing overrides this, checks are NOT ok */
    return false;   /* fail-safe default */
}


/* ============================ MAIN STARTUP FLOW ============================= */

/* Set specific permission bits to allow contactors/systems */
static void perms_set(uint32_t mask)
{
    /* Set the event flag bits specified by mask */
    osEventFlagsSet(contactorPermissionsFlagHandle, mask);
}


static void Startup_Flowchart(void)
{
    /* =========================================================================
     * FLOWCHART BLOCK: Startup Task Begins
     * - Mark state as waiting for MPS
     * =========================================================================
     */
    mbmsStatus.startupState = STARTUP_MPS_OPEN;


    /* =========================================================================
     * FLOWCHART BLOCK: Wait until MPS is ON (soft timeout)
     * - Poll MPS every POLL_DELAY_MS
     * - If timeout expires, go to fail state: "MPS timeout -> shutdown"
     * =========================================================================
     */
    {
        uint32_t start = osKernelGetTickCount();         /* capture start time */
        uint32_t limit = ms_to_ticks(MPS_SOFT_LIMIT_MS); /* convert ms to ticks */

        while (!is_mps_on())                             /* loop until MPS becomes ON */
        {
            osDelay(POLL_DELAY_MS);                      /* sleep so we don't busy-wait */

            if ((osKernelGetTickCount() - start) >= limit) /* check if timeout reached */
            {
                /* Fail State: MPS timeout -> Shutdown */
                startup_fail_shutdown();                 /* run shutdown handler */
                osThreadTerminate(startupTaskHandle);    /* stop this task immediately */
            }
        }
    }

    /* =========================================================================
     * FLOWCHART NOTE: MPS is now ON, so we can continue
     * =========================================================================
     */
    mbmsStatus.startupState = STARTUP_MPS_CLOSED;


    /* =========================================================================
     * FLOWCHART BLOCK: Wait until ESD is NOT pressed (soft timeout)
     * - Poll ESD every POLL_DELAY_MS while ESD is pressed
     * - If timeout expires, go to fail state: "ESD pressed -> shutdown"
     * =========================================================================
     */
    {
        uint32_t start = osKernelGetTickCount();         /* capture start time */
        uint32_t limit = ms_to_ticks(ESD_SOFT_LIMIT_MS); /* convert ms to ticks */

        while (is_esd_pressed())                         /* loop while ESD is pressed */
        {
            osDelay(POLL_DELAY_MS);                      /* sleep between polls */

            if ((osKernelGetTickCount() - start) >= limit) /* check if timeout reached */
            {
                /* Fail State: ESD pressed too long -> Shutdown */
                startup_fail_shutdown();                 /* run shutdown handler */
                osThreadTerminate(startupTaskHandle);    /* stop this task immediately */
            }
        }
    }


    /* =========================================================================
     * FLOWCHART BLOCK: Checks OK?
     * - Increment a counter showing checks were attempted
     * - Ask BCT if startup checks passed
     * - If checks fail, go to fail state: "Startup checks failed -> shutdown"
     * =========================================================================
     */
    mbmsStatus.startupChecksRunCount++;                  /* record that checks ran */

    if (!BCT_AreStartupChecksOk())                       /* call check function */
    {
        /* Fail State: startup checks failed -> Shutdown */
        startup_fail_shutdown();                         /* run shutdown handler */
        osThreadTerminate(startupTaskHandle);            /* stop this task */
    }


    /* =========================================================================
     * FLOWCHART BLOCK: Give permission: Common contactor
     * - Set permission flag so another task can actually close it
     * - Update startup state to show we've reached this step
     * =========================================================================
     */
    perms_set(PERM_COMMON_CONTACTOR);                    /* allow common contactor */
    mbmsStatus.startupState = STARTUP_COMMON_CLOSED;     /* update progress state */


    /* =========================================================================
     * FLOWCHART BLOCK: Give permission: LV contactor
     * =========================================================================
     */
    perms_set(PERM_LV_CONTACTOR);                        /* allow LV contactor */
    mbmsStatus.startupState = STARTUP_LV_CLOSED;         /* update progress state */


    /* =========================================================================
     * FLOWCHART BLOCK: Enable DCDC1 (switch Aux -> Main battery)
     * - Only executes if the GPIO definitions exist
     * =========================================================================
     */
#if defined(DCDC1_EN_GPIO_Port) && defined(DCDC1_EN_Pin)
    HAL_GPIO_WritePin(DCDC1_EN_GPIO_Port, DCDC1_EN_Pin, DCDC1_ENABLE_LEVEL); /* drive enable pin */
#endif
    mbmsStatus.startupState = STARTUP_DCDC1_ON;          /* record DCDC enabled step */


    /* =========================================================================
     * FLOWCHART BLOCK: Enable 12V CAN (front-of-car systems power)
     * - Only executes if the GPIO definitions exist
     * =========================================================================
     */
#if defined(CAN12V_EN_GPIO_Port) && defined(CAN12V_EN_Pin)
    HAL_GPIO_WritePin(CAN12V_EN_GPIO_Port, CAN12V_EN_Pin, CAN12V_ENABLE_LEVEL); /* drive enable pin */
#endif


    /* =========================================================================
     * FLOWCHART BLOCK: Give permission: Motor contactor
     * =========================================================================
     */
    perms_set(PERM_MOTOR_CONTACTOR);                     /* allow motor contactor(s) */
    mbmsStatus.startupState = STARTUP_MOTORS_ENABLED;    /* update progress state */


    /* =========================================================================
     * FLOWCHART BLOCK: Give permission: Array contactor
     * - Note: comment indicates we do NOT allow charging during startup
     * =========================================================================
     */
    /* We do NOT allow charging during startup */
    perms_set(PERM_ARRAY_CONTACTOR);                     /* allow array contactor */


    /* =========================================================================
     * FLOWCHART BLOCK: Startup complete (Update startup state = DONE)
     * =========================================================================
     */
    mbmsStatus.startupState = STARTUP_DONE;              /* indicate startup is complete */

    /* =========================================================================
     * FLOWCHART BLOCK: Terminate Startup Task
     * =========================================================================
     */
    osThreadTerminate(startupTaskHandle);                /* delete/terminate this task */
}


/* =============================== TASK ENTRY ================================= */

void StartupTask(void *argument)
{
    (void)argument;                                      /* unused parameter */

    /* Run the full startup sequence (matches the flowchart order) */
    Startup_Flowchart();

    /* Safety fallback in case the flow ever returns */
    osThreadTerminate(startupTaskHandle);
}
