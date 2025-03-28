#ifndef SYNC_SYNC_H
#define SYNC_SYNC_H

#include "stdint.h"
#include "common.h"

#define SYNC_TASKID_UART_OUT ((uint32_t)0)

// typedef struct {
//     uint32_t taskID;
//     uint8_t padding_[64 - sizeof(taskID)];
// } DefaultSyncTask;

// STATIC_ASSERT(sizeof(DefaultSyncTask) == 64, "");

typedef void (*task_handler_fn_t)();

int8_t SyncInit(void);
int8_t SyncRegisterHandler(task_handler_fn_t, uint32_t);

// typedef struct {
//     uint32_t taskID;
//     void *pOutBuf;
//     uint32_t len;
// } SyncTaskUARTOut;

// STATIC_ASSERT(sizeof(SyncTaskUARTOut) <= 64, "");


#endif // SYNC_SYNC_H