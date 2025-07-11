#ifndef DMA_H
#define DMA_H
#include "common.h"
#include "hal.h"
#include "log.h"
#include <stdint.h>

typedef enum {
    eDMA_DIRECTION_PERIPH_TO_MEMORY = DMA_PERIPH_TO_MEMORY,
    eDMA_DIRECTION_MEMORY_TO_PERIPH = DMA_MEMORY_TO_PERIPH,
    eDMA_DIRECTION_MEMORY_TO_MEMORY = DMA_MEMORY_TO_MEMORY
} eDMA_TRANSFER_DIR;

typedef enum {
    eDMA_PRIORITY_LOW       = DMA_PRIORITY_LOW,
    eDMA_PRIORITY_MEDIUM    = DMA_PRIORITY_MEDIUM,
    eDMA_PRIORITY_HIGH      = DMA_PRIORITY_HIGH,
    eDMA_PRIORITY_VERY_HIGH = DMA_PRIORITY_VERY_HIGH
} eDMA_PRIORITY;

typedef struct {
    DMA_Stream_TypeDef* pDMA;
    eDMA_TRANSFER_DIR direction;
    eDMA_PRIORITY priority;
} DMAConfig;

// Forward declaration of the PWM DMA handle structure
// typedef struct __PWMDMAHandle PWMDMAHandle;

STATUS_TYPE DMASystemInit (void);
STATUS_TYPE DMAInit (DMAConfig* pConfig, DMA_HandleTypeDef** ppOutHandle);

#endif /* DMA_H */