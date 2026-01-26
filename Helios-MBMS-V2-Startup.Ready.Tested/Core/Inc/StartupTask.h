#pragma once
#include <stdbool.h>
#include <stdint.h>


// Change when electrical tells you to.
#define MAIN_CONTACTOR_ON_ACTIVE 1
#define COMMON_CONTACTOR_ON_ACTIVE 1



/* ============================ STARTUP STATES ============================ */
typedef enum
{
    STARTUP_START = 0,              // NEW: Startup begins

    STARTUP_MPS_OPEN,               // waiting for MPS ON (naming kept)
    STARTUP_MPS_CLOSED,             // MPS is ON

    STARTUP_ESD_WAITING,            // NEW: waiting for ESD release
    STARTUP_ESD_RELEASED,           // NEW: ESD is NOT pressed

	MAIN_COMMON_CLOSED,

    STARTUP_CHECKS_COMPLETED,       // NEW: completed startup checks

    STARTUP_LV_CLOSED,              // LV permission granted

    STARTUP_DCDC1_ON,               // DCDC1 enabled
    STARTUP_CAN12V_ON,              // NEW: 12V CAN enabled

    STARTUP_MOTORS_ENABLED,         // motor permission granted
    STARTUP_ARRAY_ENABLED,          // NEW: array permission granted

    STARTUP_DONE                   // startup done
} StartupState_t;





/* ============================ MBMS STATUS ============================ */
typedef struct
{
    volatile StartupState_t startupState;

    /* Keep if you want the field, but DO NOT increment it in StartupTask anymore */
    volatile uint32_t startupChecksRunCount;

} MBMS_Status_t;

extern MBMS_Status_t mbmsStatus;





/* ============================ PERMISSIONS (NO FLAGS) ============================ */
/* StartupTask writes these; other tasks read these. */
typedef struct
{
    volatile uint8_t allow_common;
    volatile uint8_t allow_lv;
    volatile uint8_t allow_motor;
    volatile uint8_t allow_array;
} ContactorPermissions_t;

extern ContactorPermissions_t gContactorPerms;

/* BatteryControlTask provides this (real implementation later) */
bool BCT_AreStartupChecksOk(void);

/* FreeRTOS/CMSIS task entry */
void StartupTask(void *argument);
