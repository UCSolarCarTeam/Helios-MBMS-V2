#pragma once
#include <stdbool.h>
#include <stdint.h>






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

    STARTUP_LV_ENABLED,              // LV permission granted

    STARTUP_DCDC1_ON,               // DCDC1 enabled
    STARTUP_12V_CAN_ON,              // NEW: 12V CAN enabled

    STARTUP_MOTORS_ENABLED,         // motor permission granted
    STARTUP_ARRAY_ENABLED,          // NEW: array permission granted

    STARTUP_DONE                   // startup done
} StartupState_t;


/* BatteryControlTask provides this (real implementation later) */
bool BCT_AreStartupChecksOk(void);

/* FreeRTOS/CMSIS task entry */
void StartupTask(void *argument);













