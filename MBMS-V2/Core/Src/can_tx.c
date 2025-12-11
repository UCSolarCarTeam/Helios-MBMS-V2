#include "can_tx.h"
#include "cmsis_os2.h"

extern FDCAN_HandleTypeDef hfdcan1;

extern osMessageQueueId_t canTxQueueHandle;

