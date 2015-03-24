
/*
 *  A circular buffer implementation for Rte fifos.*
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "Os.h"
#include "Rte_Fifo.h"

#if defined(__GNUC__)
#define MEMCPY(_x,_y,_z)	__builtin_memcpy(_x,_y,_z)
#else
#define MEMCPY(_x,_y,_z)	memcpy(_x,_y,_z)
#endif

/* IMPROVMENT: Make it threadsafe, add DisableAllInterrts()/EnableAllInterrupts() */
void Rte_Fifo_Init(RteFifoType *fBuf, void *buffer, int maxCnt, size_t dataSize) {
    fBuf->bufStart = buffer;
    fBuf->maxCnt = maxCnt;
    fBuf->bufEnd = (char *) fBuf->bufStart + dataSize * maxCnt;
    fBuf->head = fBuf->bufStart;
    fBuf->tail = fBuf->bufStart;
    fBuf->dataSize = dataSize;
    fBuf->currCnt = 0;
}

Std_ReturnType Rte_Fifo_Push(RteFifoType *fPtr, void const *dataPtr) {
    SYS_CALL_SuspendAllInterrupts();
    if ((fPtr->currCnt == fPtr->maxCnt) || (fPtr == NULL)) {
        SYS_CALL_ResumeAllInterrupts();
        fPtr->bufFullFlag = true;
        return RTE_E_LIMIT; /* No more room */
    }
    MEMCPY(fPtr->head, dataPtr, fPtr->dataSize);
    fPtr->head = (char *) fPtr->head + fPtr->dataSize;
    if (fPtr->head == fPtr->bufEnd) {
        fPtr->head = fPtr->bufStart;
    }
    ++fPtr->currCnt;
    SYS_CALL_ResumeAllInterrupts();

    return RTE_E_OK;
}

/**
 * Pop an entry from the buffer.
 *
 * @param fPtr    Pointer to the queue created with CirqBuffStatCreate, etc.
 * @param dataPtr
 * @return RTE_E_OK - if successfully popped.
 *         RTE_E_NO_DATA - nothing popped (it was empty)
 *         RTE_E_LOST_DATA - if a buffer overflow has occurred previously
 */
Std_ReturnType Rte_Fifo_Pop(RteFifoType *fPtr, void *dataPtr) {
    SYS_CALL_SuspendAllInterrupts();
    if ((fPtr->currCnt == 0) || (fPtr == NULL)) {
        SYS_CALL_ResumeAllInterrupts();
        return RTE_E_NO_DATA;
    }
    MEMCPY(dataPtr, fPtr->tail, fPtr->dataSize);
    fPtr->tail = (char *) fPtr->tail + fPtr->dataSize;
    if (fPtr->tail == fPtr->bufEnd) {
        fPtr->tail = fPtr->bufStart;
    }
    --fPtr->currCnt;

    if (fPtr->bufFullFlag) {
        fPtr->bufFullFlag = false;
        SYS_CALL_ResumeAllInterrupts();
        return RTE_E_LOST_DATA;
    }

    SYS_CALL_ResumeAllInterrupts();
    return RTE_E_OK;

}

