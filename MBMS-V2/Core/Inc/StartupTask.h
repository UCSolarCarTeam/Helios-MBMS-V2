#ifndef STARTUP_TASK_H
#define STARTUP_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* ============================================================================
 * StartupTask.h
 *
 * Provides:
 *  - Startup task entry function
 *  - Startup state tracking (for other tasks to read)
 *  - Permission bit masks for contactorPermissionsFlagHandle
 *
 * This matches the linear startup flowchart:
 *   MPS wait -> ESD wait -> checks -> perms -> DONE -> terminate task
 * ============================================================================
 */


/* ============================ STARTUP STATE ENUM ============================ */

typedef enum
{
    STARTUP_MPS_OPEN = 0,        /* waiting for MPS to become ON */
    STARTUP_MPS_CLOSED,          /* MPS is ON */

    STARTUP_COMMON_CLOSED,       /* permission given: common contactor */
    STARTUP_LV_CLOSED,           /* permission given: LV contactor */

    STARTUP_DCDC1_ON,            /* DCDC1 enabled (aux -> main) */
    STARTUP_MOTORS_ENABLED,      /* permission given: motor contactor(s) */

    STARTUP_DONE                /* startup complete */
} StartupState_t;


/* ============================== GLOBAL STATUS =============================== */

typedef struct
{
    StartupState_t startupState;         /* current startup step */
    uint32_t       startupChecksRunCount;/* how many times checks were attempted */
} MBMS_Status_t;

/* Global status instance (defined in StartupTask.c) */
extern MBMS_Status_t mbmsStatus;


/* ============================ PERMISSION BIT MASKS ==========================
 * These are used with:
 *   osEventFlagsSet(contactorPermissionsFlagHandle, mask)
 *
 * IMPORTANT:
 * These are placeholders if you don't already have official definitions.
 * If your project already defines these elsewhere, remove or align them
 * to avoid duplicate-definition errors.
 * ============================================================================
 */

#ifndef PERM_COMMON_CONTACTOR
#define PERM_COMMON_CONTACTOR   (1UL << 0)
#endif

#ifndef PERM_LV_CONTACTOR
#define PERM_LV_CONTACTOR       (1UL << 1)
#endif

#ifndef PERM_MOTOR_CONTACTOR
#define PERM_MOTOR_CONTACTOR    (1UL << 2)
#endif

#ifndef PERM_ARRAY_CONTACTOR
#define PERM_ARRAY_CONTACTOR    (1UL << 3)
#endif


/* ============================== TASK FUNCTION =============================== */

/* FreeRTOS/CMSIS-RTOS task entry function */
void StartupTask(void *argument);


/* ============================ STARTUP CHECK HOOK ============================
 * Implement this in BatteryControlTask (BCT) later to run real checks.
 * The version in StartupTask.c is weak, so your BCT definition will override it.
 * ============================================================================
 */
bool BCT_AreStartupChecksOk(void);


#ifdef __cplusplus
}
#endif

#endif /* STARTUP_TASK_H */
