
/*
 * Generator version: 1.0.0
 * AUTOSAR version:   4.0.3
 */

#include "os_i.h"


// ###############################    EXTERNAL REFERENCES    #############################
 
/* Application externals */




/* Interrupt externals */

// Set the os tick frequency
OsTickType OsTickFreq = 1000;


// ###############################    DEBUG OUTPUT     #############################
uint32 os_dbg_mask = D_EVENT |D_TASK |D_ALARM | 0;                     


// ###############################    APPLICATIONS     #############################
GEN_APPLICATION_HEAD = {

	GEN_APPLICATION(
				/* id           */ APPLICATION_ID_OsApplication,
				/* name         */ "OsApplication",
				/* trusted      */ true,
				/* core         */ 0,
				/* StartupHook  */ NULL,
				/* ShutdownHook */ NULL,
				/* ErrorHook    */ NULL,
				/* rstrtTaskId  */ 0 // NOT CONFIGURABLE IN TOOLS (OsTasks.indexOf(app.Os RestartTask.value))
				)

};


 


// #################################    COUNTERS     ###############################

GEN_COUNTER_HEAD = {
	GEN_COUNTER(
				/* id          */		COUNTER_ID_OsCounter,
				/* name        */		"OsCounter",
				/* counterType */		COUNTER_TYPE_HARD,
				/* counterUnit */		COUNTER_UNIT_NANO,
				/* maxAllowed  */		0xffff,
				/*             */		1,
				/* minCycle    */		1,
				/*             */		0,
				/* owningApp   */		APPLICATION_ID_OsApplication,
				/* accAppMask..*/       (1 << APPLICATION_ID_OsApplication)
						                | (1 << APPLICATION_ID_OsApplication) 
 ) 
};




CounterType Os_Arc_OsTickCounter = COUNTER_ID_OsCounter;




// ##################################    ALARMS     ################################
GEN_ALARM_AUTOSTART(
				ALARM_ID_Alarm_Main,
				ALARM_AUTOSTART_RELATIVE,
				10,
				0,
				OSDEFAULTAPPMODE );

GEN_ALARM_AUTOSTART(
				ALARM_ID_StepAlarm,
				ALARM_AUTOSTART_ABSOLUTE,
				100,
				100,
				OSDEFAULTAPPMODE );

GEN_ALARM_AUTOSTART(
				ALARM_ID_CriticalAlarm,
				ALARM_AUTOSTART_ABSOLUTE,
				50,
				50,
				OSDEFAULTAPPMODE );




GEN_ALARM_HEAD = {

	GEN_ALARM(	ALARM_ID_Alarm_Main,
				"Alarm_Main",
				COUNTER_ID_OsCounter,
				GEN_ALARM_AUTOSTART_NAME(ALARM_ID_Alarm_Main),
				ALARM_ACTION_SETEVENT,
				TASK_ID_OsTask_Main,
				EVENT_MASK_OsEvent_Main,
				0,
				APPLICATION_ID_OsApplication, /* Application owner */
				( 1 << APPLICATION_ID_OsApplication)
						                | (1 << APPLICATION_ID_OsApplication) 
 /* Accessing application mask */
			)
,
	GEN_ALARM(	ALARM_ID_StepAlarm,
				"StepAlarm",
				COUNTER_ID_OsCounter,
				GEN_ALARM_AUTOSTART_NAME(ALARM_ID_StepAlarm),
				ALARM_ACTION_SETEVENT,
				TASK_ID_StepTask,
				EVENT_MASK_StepEvent,
				0,
				APPLICATION_ID_OsApplication, /* Application owner */
				( 1 << APPLICATION_ID_OsApplication)
						                | (1 << APPLICATION_ID_OsApplication) 
 /* Accessing application mask */
			)
,
	GEN_ALARM(	ALARM_ID_CriticalAlarm,
				"CriticalAlarm",
				COUNTER_ID_OsCounter,
				GEN_ALARM_AUTOSTART_NAME(ALARM_ID_CriticalAlarm),
				ALARM_ACTION_SETEVENT,
				TASK_ID_CriticalTask,
				EVENT_MASK_CriticalEvent,
				0,
				APPLICATION_ID_OsApplication, /* Application owner */
				( 1 << APPLICATION_ID_OsApplication)
						                | (1 << APPLICATION_ID_OsApplication) 
 /* Accessing application mask */
			)

};

 
// ################################    RESOURCES     ###############################


// ##############################    STACKS (TASKS)     ############################

DECLARE_STACK(OsIdle, OS_OSIDLE_STACK_SIZE);


DECLARE_STACK(CriticalTask,  2048);
DECLARE_STACK(OsTask_Main,  2048);
DECLARE_STACK(StepTask,  2048);


// ##################################    TASKS     #################################


// Linker symbols defined for Non-Trusted Applications

GEN_TASK_HEAD = {
	
	{
	.pid = TASK_ID_OsIdle,
	.name = "OsIdle",
	.entry = OsIdle,
	.prio = 0,
	.scheduling = FULL,
	.autostart = TRUE,
	.proc_type = PROC_BASIC,
	.stack.size = sizeof stack_OsIdle,
	.stack.top = stack_OsIdle,
	.resourceAccess = 0,
	.activationLimit = 1,
	.applOwnerId = OS_CORE_0_MAIN_APPLICATION,
	.accessingApplMask = (1 << OS_CORE_0_MAIN_APPLICATION),
	},
	



{
	.pid = TASK_ID_CriticalTask,
	.name = "CriticalTask",
	.entry = CriticalTask,
	.prio = 5,
	.scheduling = FULL,
	.proc_type = PROC_EXTENDED,
	.stack.size = sizeof stack_CriticalTask,
	.stack.top = stack_CriticalTask,
	.autostart = TRUE,
	.resourceAccess = 0 , 
	.activationLimit = 1,
	.eventMask = 0 | EVENT_MASK_CriticalEvent ,
	
	.applOwnerId = APPLICATION_ID_OsApplication,
	.accessingApplMask = (1 <<APPLICATION_ID_OsApplication)
,
},
		

{
	.pid = TASK_ID_OsTask_Main,
	.name = "OsTask_Main",
	.entry = OsTask_Main,
	.prio = 1,
	.scheduling = FULL,
	.proc_type = PROC_EXTENDED,
	.stack.size = sizeof stack_OsTask_Main,
	.stack.top = stack_OsTask_Main,
	.autostart = TRUE,
	.resourceAccess = 0 , 
	.activationLimit = 1,
	.eventMask = 0 | EVENT_MASK_OsEvent_Main ,
	
	.applOwnerId = APPLICATION_ID_OsApplication,
	.accessingApplMask = (1 <<APPLICATION_ID_OsApplication)
,
},
		

{
	.pid = TASK_ID_StepTask,
	.name = "StepTask",
	.entry = StepTask,
	.prio = 3,
	.scheduling = FULL,
	.proc_type = PROC_EXTENDED,
	.stack.size = sizeof stack_StepTask,
	.stack.top = stack_StepTask,
	.autostart = TRUE,
	.resourceAccess = 0 , 
	.activationLimit = 1,
	.eventMask = 0 | EVENT_MASK_StepEvent ,
	
	.applOwnerId = APPLICATION_ID_OsApplication,
	.accessingApplMask = (1 <<APPLICATION_ID_OsApplication)
,
},
		

};

// ##################################    HOOKS     #################################
GEN_HOOKS( 
	StartupHook, 
	NULL,
	ShutdownHook, 
 	ErrorHook,
 	NULL,
 	NULL 
);

// ##################################    ISRS     ##################################



GEN_ISR_MAP = {
  0
};

// ############################    SCHEDULE TABLES     #############################


 
 // ############################    SPINLOCKS     ##################################



