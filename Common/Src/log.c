#include <stdio.h>
#include <string.h>
#include "stm32h7xx_hal.h"

#include "log.h"
#include "mem/mem.h"
#include "mem/ring_buff.h"
#include "sync/mailbox.h"
#include "sync/sync.h"

#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif

RingBuff volatile *pCM4RingBuf;
RingBuff volatile *pCM7RingBuf;
UART_HandleTypeDef *pUART;

static void SyncUARTTaskHandler(void);
static int8_t LoggerWriteToUART(RingBuff volatile *pBuff);

PUTCHAR_PROTOTYPE
{
  if(HAL_GetCurrentCPUID() == CM7_CPUID)
  {
    RingBuffWrite(pCM7RingBuf, (void*)&ch, 1);
    if((char)ch == '\n')
    {
      LoggerWriteToUART(pCM7RingBuf);
    }
  }
  else 
  {
    RingBuffWrite(pCM4RingBuf, (void*)&ch, 1);
    if((char)ch == '\n')
    {
      /*
      * CM4 sends signal to CM7 to send CM4's ring buffer to the UART interface
      */
      uint32_t taskID = SYNC_TASKID_UART_OUT;
      SyncMailBoxWriteNotify(MAILBOX_CM4_ID, (uint8_t*)&taskID, sizeof(uint32_t));
    }
  }
  return ch;
}

int8_t LoggerInit(UART_HandleTypeDef *pUART_)
{
  if(HAL_GetCurrentCPUID() != CM7_CPUID || pUART_ == NULL) 
  {
    pUART = NULL;
    return -1;
  }

  pCM4RingBuf = RingBuffCreate((void*)MEM_SHARED_CM4_UART_RINGBUFF_START, MEM_SHARED_CM4_UART_RINGBUFF_TOTAL_LEN);
  pCM7RingBuf = RingBuffCreate((void*)MEM_SHARED_CM7_UART_RINGBUFF_START, MEM_SHARED_CM7_UART_RINGBUFF_TOTAL_LEN);
  pUART = pUART_;
  SyncRegisterHandler(SyncUARTTaskHandler, SYNC_TASKID_UART_OUT);

  return 0;
}

static int8_t LoggerWriteToUART(RingBuff volatile *pRingBuf)
{
  uint32_t len = 0;
  void *pBufToTransmit = NULL;

  send:
  len = RingBuffGetLinearBlockReadLength(pRingBuf);
  if(len > 0)
  {
    pBufToTransmit = RingBuffGetLinearBlockReadAddress(pRingBuf);
    HAL_UART_Transmit(pUART, pBufToTransmit, len, 1000);
    RingBuffSkip(pRingBuf, len);
    /* Check for anything in the overflow buffer */
    goto send;
  }
  return 0;
}

static void SyncUARTTaskHandler(void)
{
  if(HAL_GetCurrentCPUID() == CM7_CPUID)
  {
    LoggerWriteToUART(pCM4RingBuf);
  }
}