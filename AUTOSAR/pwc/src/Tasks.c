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

#include "Os.h"
#include "debug.h"
#include "Port.h"
#include "Ecum.h"
#include "stdio.h"

#include "PWC.h"

extern const Dio_ChannelType DioChannelConfigData[];
extern void EcuM_enter_run_mode(void);
//extern uint8_t IOE_Config();

/** Blink Leds */
void OsTask_Main( void ) {

    LDEBUG_PRINTF("## OsTask_Main\n");

    // Check if the IOExpander is working for the switch
    if (IOE_Config() == IOE_OK)
    {
    	IoHwAb_Digital_Write_MOTOR_1(IOHWAB_HIGH);
    	IoHwAb_Digital_Write_MOTOR_2(IOHWAB_HIGH);
    }
}

/** All other OS hooks */
void OsIdle( void ) {
    while(1){}
}

// Hooks
#define ERROR_LOG_SIZE 20

struct LogBad_s {
    uint32_t param1;
    uint32_t param2;
    uint32_t param3;
    TaskType taskId;
    OsServiceIdType serviceId;
    StatusType error;
};

void ErrorHook ( StatusType Error ) {

    TaskType task;
    static struct LogBad_s LogBad[ERROR_LOG_SIZE];
    static uint8_t ErrorCount = 0;
    GetTaskID(&task);
    OsServiceIdType service = OSErrorGetServiceId();

    LDEBUG_PRINTF("## ErrorHook err=%d\n",Error);

    /* Log the errors in a buffer for later review */
    LogBad[ErrorCount].param1 = os_error.param1;
    LogBad[ErrorCount].param2 = os_error.param2;
    LogBad[ErrorCount].param3 = os_error.param3;
    LogBad[ErrorCount].serviceId = service;
    LogBad[ErrorCount].taskId = task;
    LogBad[ErrorCount].error = Error;

    ErrorCount++;

    // Stall if buffer is full.
    while(ErrorCount >= ERROR_LOG_SIZE){}

}

void PostTaskHook ( void ) {
    TaskType task;
    GetTaskID(&task);
    LDEBUG_PRINTF("## PreTaskHook, taskid=%d\n",task);
}

void PreTaskHook ( void ) {
    TaskType task;
    GetTaskID(&task);
    LDEBUG_PRINTF("## PreTaskHook, taskid=%d\n",task);
}

void ShutdownHook ( StatusType Error ) {
    LDEBUG_PRINTF("## ShutdownHook\n");
    while(1){}
    (void) Error;
}

void StartupHook ( void ) {
    LDEBUG_PRINTF("## StartupHook\n");
}
