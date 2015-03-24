/*-------------------------------- Arctic Core ------------------------------
 * Copyright (C) 2013, ArcCore AB, Sweden, www.arccore.com.
 * Contact: <contact@arccore.com>
 *
 * You may ONLY use this file:
 * 1)if you have a valid commercial ArcCore license and then in accordance with
 * the terms contained in the written license agreement between you and ArcCore,
 * or alternatively
 * 2)if you follow the terms found in GNU General Public License version 2 as
 * published by the Free Software Foundation and appearing in the file
 * LICENSE.GPL included in the packaging of this file or here
 * <http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt>
 *-------------------------------- Arctic Core -----------------------------*/



/*
 * Mutex usage:
 * - Serialized access to a global resource. The same task
 *   ALWAYS first use Os_WaitMutex() and the Os_ReleaseMutex()
 *   Should mutex be as GetResource()/ReleaseResource() be able to
 *   tinker with priotities? Yeees, raise the priority to the
 *   priority of the calling task ( Second call to WaitMutex() )
 *
 *
 *   Task1:
 *     Os_WaitMutex(my_mutex)
 *     ..
 *     Os_ReleaseMutex(my_mutex)

 *   Task2
 *     Os_WaitMutex(my_mutex)
 *     ..
 *     Os_ReleaseMutex(my_mutex)
 *
 * Semaphore usage:
 * - Producer and consumer problems.
 *   Os_WaitSemaphore() and Os_SignalSemaphore().
 *
 *   ISR:
 *     Os_SignalSemaphore(my_semaphore)

 *   Task:
 *     Os_WaitSemaphore(my_semaphore)
 *
 *
 * Mutex vs Resource's
 *   A task, say TASK_A calls GetResource(RES_1) any task that has anything to do
 *   with the RES_1 will not be able to run (since the priority of TASK_A will be
 *   raised to the priority of the task with the highest priority accessing the
 *   resource). This does not encourage the use of resources that is scarcely used
 *   within a high priority task (since it will not get runtime when the resource is
 *   held).
 *
 *   Mutex:es on the other hand will make other tasks run that shares the resource
 *   but the priority of the task that take the mutex for the second time makes the
 *   first task inherit the priority of that task.
 *
 *   Resources is deadlock safe while mutex:es are not (circular lock dependencies)
 */

/* ----------------------------[includes]------------------------------------*/
#include "os_i.h"


/* ----------------------------[private define]------------------------------*/
/* ----------------------------[private macro]-------------------------------*/
/* ----------------------------[private typedef]-----------------------------*/
/* ----------------------------[private function prototypes]-----------------*/
/* ----------------------------[private variables]---------------------------*/
/* ----------------------------[private functions]---------------------------*/
/* ----------------------------[public functions]----------------------------*/


/**
 * Wait for a semaphore.
 *
 * The value of the semaphore is decremented. If the value is negative the
 * caller is put in WAITING state.
 *
 * If the scheduler is locked (hold RES_SCHEDULER) the call will fail and
 * return E_NOT_OK.
 *
 * @param semPtr
 * @param tmo     A timeout of 0 can be used for polling the semaphore.
 * @return
 */

StatusType WaitSemaphore( OsSemaphoreType *semPtr, TickType tmo ) {
	OsTaskVarType *pcbPtr;
	uint32_t flags;
	StatusType rv = E_OK;
	(void)tmo;

	Irq_Save(flags);

	--semPtr->val;

	pcbPtr = Os_SysTaskGetCurr();
	assert(OS_SYS_PTR->intNestCnt == 0 );

	if (pcbPtr->constPtr->proc_type != PROC_EXTENDED) {
		return E_OS_ACCESS;
	}

	if(semPtr->val < 0 ) {
		/* To WAITING state */
		if( tmo == 0 ) {
			/* Failed to acquire the semaphore */
			rv = E_NOT_OK;
		} else {
			/* Add this task to the semaphore */
			STAILQ_INSERT_TAIL(&semPtr->taskHead,pcbPtr,semEntry);

			Os_Dispatch(OP_WAIT_SEMAPHORE);
		}
	} else {
		/* We got the semaphore */
		if( tmo == 0 ) {

			rv = E_NOT_OK;
		} else {
			/*
			 * Wait for the semaphore to be signaled or timeout
			 */
			if ( Os_SchedulerResourceIsFree() ) {
				/* Set the timeout */
				if( tmo != TICK_MAX ) {
					TAILQ_INSERT_TAIL(&OS_SYS_PTR->timerHead,pcbPtr,timerEntry);
					pcbPtr->timerDec = tmo;
				}

				Os_Dispatch(OP_WAIT_SEMAPHORE);
			} else {
				/* We hold RES_SCHEDULER */
				rv = E_NOT_OK;
			}
		}
	}

	Irq_Restore(flags);

	return rv;
}


/**
 * Increase the semaphore value by 1. If
 *
 * @param semPtr
 */
void SignalSemaphore( OsSemaphoreType *semPtr ) {
	uint32_t flags;
	OsTaskVarType *taskPtr;

	Irq_Save(flags);

	assert( semPtr != NULL );

	++semPtr->val;

	/* Remove the first task that waits at the semaphore */
	if( semPtr->val <= 0 ) {

		taskPtr = STAILQ_FIRST(&semPtr->taskHead);
		/* Make the first task ready */
		Os_TaskMakeReady(taskPtr);

		/* Release the first task in queue */
		STAILQ_REMOVE_HEAD(&semPtr->taskHead,semEntry);

		if( taskPtr->activePriority > Os_SysTaskGetCurr()->activePriority ) {
			Os_Dispatch(OP_SIGNAL_SEMAPHORE);
		}
	}

	Irq_Restore(flags);
}


/*
 * Usage:
 */
StatusType InitSemaphore( OsSemaphoreType *semPtr, int initialCount  ) {

	semPtr->val = initialCount;
	STAILQ_INIT(&semPtr->taskHead);
	return E_OK;
}

#if 0

/* With priority inheretance */
StatusType CreateMutex( OsMutexType *mutexPtr ) {


}
#endif

/**
 *
 * @param mutexPtr
 * @return
 */

StatusType WaitMutex( OsMutexType *mutexPtr ) {

	(void)mutexPtr;
	return E_OK;
}

StatusType ReleaseMutex( OsMutexType *mutexPtr ) {

	(void)mutexPtr;
	return E_OK;
}









