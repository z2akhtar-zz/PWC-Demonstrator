#include <Rte_Internal.h>
#include <Rte_Calprms.h>
#include <Rte_Assert.h>
#include <Rte_Fifo.h>
#include <Com.h>
#include <Os.h>
#include <Ioc.h>

/** --- EXTERNALS --------------------------------------------------------------------------- */
extern Std_ReturnType Rte_ioHwAb_DigitalWrite(/*IN*/IoHwAb_SignalType_ portDefArg1, /*IN*/DigitalLevel Level);
extern Std_ReturnType Rte_ioHwAb_DigitalRead(/*IN*/IoHwAb_SignalType_ portDefArg1, /*OUT*/DigitalLevel * Level, /*OUT*/SignalQuality * Quality);

/** --- RTE INTERNAL DATA ------------------------------------------------------------------- */
#define Rte_START_SEC_VAR_INIT_UNSPECIFIED
#include <Rte_MemMap.h>
boolean RteInitialized = false;
#define Rte_STOP_SEC_VAR_INIT_UNSPECIFIED
#include <Rte_MemMap.h>

/** === EndStopDtcType Data =============================================================== 
 */

/** === IoHwAb Data =============================================================== 
 */

/** === MotorDriverType Data =============================================================== 
 */
#define MotorDriverType_START_SEC_VAR_INIT_UNSPECIFIED
#include <MotorDriverType_MemMap.h>

commandType Rte_Buffer_MotorDriver_cmd_command;
#define MotorDriverType_STOP_SEC_VAR_INIT_UNSPECIFIED
#include <MotorDriverType_MemMap.h>

/** === ObstacleDtcType Data =============================================================== 
 */

/** === SwitchType Data =============================================================== 
 */

/** === WinArbitratorType Data =============================================================== 
 */
#define WinArbitratorType_START_SEC_VAR_INIT_UNSPECIFIED
#include <WinArbitratorType_MemMap.h>

requestType Rte_Buffer_WinArbitrator_req_d_request;
#define WinArbitratorType_STOP_SEC_VAR_INIT_UNSPECIFIED
#include <WinArbitratorType_MemMap.h>

/** === WinControllerType Data =============================================================== 
 */
#define WinControllerType_START_SEC_VAR_INIT_UNSPECIFIED
#include <WinControllerType_MemMap.h>

myBoolean Rte_Buffer_WinController_endStop_isPresent;
#define WinControllerType_STOP_SEC_VAR_INIT_UNSPECIFIED
#include <WinControllerType_MemMap.h>
#define WinControllerType_START_SEC_VAR_INIT_UNSPECIFIED
#include <WinControllerType_MemMap.h>

myBoolean Rte_Buffer_WinController_obstacle_isPresent;
#define WinControllerType_STOP_SEC_VAR_INIT_UNSPECIFIED
#include <WinControllerType_MemMap.h>
#define WinControllerType_START_SEC_VAR_INIT_UNSPECIFIED
#include <WinControllerType_MemMap.h>

requestType Rte_Buffer_WinController_req_request;
#define WinControllerType_STOP_SEC_VAR_INIT_UNSPECIFIED
#include <WinControllerType_MemMap.h>

/** --- SERVER ACTIVATIONS ------------------------------------------------------------------ */

/** --- FUNCTIONS --------------------------------------------------------------------------- */
#define Rte_START_SEC_CODE
#include <Rte_MemMap.h>

/** === EndStopDtcType ======================================================================= */
/** --- EndStopDtc -------------------------------------------------------------------- */

/** ------ endStop */

Std_ReturnType Rte_Write_EndStopDtcType_EndStopDtc_endStop_isPresent(/*IN*/myBoolean value) {
    Std_ReturnType retVal = RTE_E_OK;

    /* --- Sender (SR9) */
    {
        SYS_CALL_SuspendAllInterrupts();
        Rte_Buffer_WinController_endStop_isPresent = value;

        SYS_CALL_ResumeAllInterrupts();
    }

    return retVal;
}

/** ------ isEndStop */
Std_ReturnType Rte_Call_EndStopDtcType_EndStopDtc_isEndStop_Read(/*OUT*/DigitalLevel * Level, /*OUT*/SignalQuality * Quality) {
    return Rte_Call_IoHwAb_ioHwAb_Digital_DigitalSignal_EndStop_Read(Level, Quality);
}

#define Rte_STOP_SEC_CODE
#include <Rte_MemMap.h>
#define Rte_START_SEC_CODE
#include <Rte_MemMap.h>

/** === IoHwAb ======================================================================= */
/** --- ioHwAb -------------------------------------------------------------------- */

/** ------ Digital_DigitalSignal_EndStop */
Std_ReturnType Rte_Call_IoHwAb_ioHwAb_Digital_DigitalSignal_EndStop_Read(/*OUT*/DigitalLevel * Level, /*OUT*/SignalQuality * Quality) {
    return Rte_ioHwAb_DigitalRead(1, Level, Quality);
}

/** ------ Digital_DigitalSignal_LED1 */
Std_ReturnType Rte_Call_IoHwAb_ioHwAb_Digital_DigitalSignal_LED1_Write(/*IN*/DigitalLevel Level) {
    return Rte_ioHwAb_DigitalWrite(0, Level);
}

/** ------ Digital_DigitalSignal_LED2 */
Std_ReturnType Rte_Call_IoHwAb_ioHwAb_Digital_DigitalSignal_LED2_Write(/*IN*/DigitalLevel Level) {
    return Rte_ioHwAb_DigitalWrite(3, Level);
}

/** ------ Digital_DigitalSignal_Obstacle */
Std_ReturnType Rte_Call_IoHwAb_ioHwAb_Digital_DigitalSignal_Obstacle_Read(/*OUT*/DigitalLevel * Level, /*OUT*/SignalQuality * Quality) {
    return Rte_ioHwAb_DigitalRead(2, Level, Quality);
}

#define Rte_STOP_SEC_CODE
#include <Rte_MemMap.h>
#define Rte_START_SEC_CODE
#include <Rte_MemMap.h>

/** === MotorDriverType ======================================================================= */
/** --- MotorDriver -------------------------------------------------------------------- */

/** ------ RunMotor_1 */
Std_ReturnType Rte_Call_MotorDriverType_MotorDriver_RunMotor_1_Write(/*IN*/DigitalLevel Level) {
    return Rte_Call_IoHwAb_ioHwAb_Digital_DigitalSignal_LED1_Write(Level);
}

/** ------ RunMotor_2 */
Std_ReturnType Rte_Call_MotorDriverType_MotorDriver_RunMotor_2_Write(/*IN*/DigitalLevel Level) {
    return Rte_Call_IoHwAb_ioHwAb_Digital_DigitalSignal_LED2_Write(Level);
}

/** ------ cmd */
Std_ReturnType Rte_Read_MotorDriverType_MotorDriver_cmd_command(/*OUT*/commandType * value) {
    Std_ReturnType status = RTE_E_OK;

    /* --- Receiver (SR7) */
    {
        SYS_CALL_SuspendAllInterrupts();
        *value = Rte_Buffer_MotorDriver_cmd_command;
        SYS_CALL_ResumeAllInterrupts();
    }

    return status;
}

#define Rte_STOP_SEC_CODE
#include <Rte_MemMap.h>
#define Rte_START_SEC_CODE
#include <Rte_MemMap.h>

/** === ObstacleDtcType ======================================================================= */
/** --- ObstacleDtc -------------------------------------------------------------------- */

/** ------ isObstacle */
Std_ReturnType Rte_Call_ObstacleDtcType_ObstacleDtc_isObstacle_Read(/*OUT*/DigitalLevel * Level, /*OUT*/SignalQuality * Quality) {
    return Rte_Call_IoHwAb_ioHwAb_Digital_DigitalSignal_Obstacle_Read(Level, Quality);
}

/** ------ obstacle */

Std_ReturnType Rte_Write_ObstacleDtcType_ObstacleDtc_obstacle_isPresent(/*IN*/myBoolean value) {
    Std_ReturnType retVal = RTE_E_OK;

    /* --- Sender (SR8) */
    {
        SYS_CALL_SuspendAllInterrupts();
        Rte_Buffer_WinController_obstacle_isPresent = value;

        SYS_CALL_ResumeAllInterrupts();
    }

    return retVal;
}

#define Rte_STOP_SEC_CODE
#include <Rte_MemMap.h>
#define Rte_START_SEC_CODE
#include <Rte_MemMap.h>

/** === SwitchType ======================================================================= */
/** --- Switch -------------------------------------------------------------------- */

/** ------ req */

Std_ReturnType Rte_Write_SwitchType_Switch_req_request(/*IN*/requestType value) {
    Std_ReturnType retVal = RTE_E_OK;

    /* --- Sender (SR5) */
    {
        SYS_CALL_SuspendAllInterrupts();
        Rte_Buffer_WinArbitrator_req_d_request = value;

        SYS_CALL_ResumeAllInterrupts();
    }

    return retVal;
}

#define Rte_STOP_SEC_CODE
#include <Rte_MemMap.h>
#define Rte_START_SEC_CODE
#include <Rte_MemMap.h>

/** === WinArbitratorType ======================================================================= */
/** --- WinArbitrator -------------------------------------------------------------------- */

/** ------ req_a */

Std_ReturnType Rte_Write_WinArbitratorType_WinArbitrator_req_a_request(/*IN*/requestType value) {
    Std_ReturnType retVal = RTE_E_OK;

    /* --- Sender (SR6) */
    {
        SYS_CALL_SuspendAllInterrupts();
        Rte_Buffer_WinController_req_request = value;

        SYS_CALL_ResumeAllInterrupts();
    }

    return retVal;
}

/** ------ req_d */
Std_ReturnType Rte_Read_WinArbitratorType_WinArbitrator_req_d_request(/*OUT*/requestType * value) {
    Std_ReturnType status = RTE_E_OK;

    /* --- Receiver (SR5) */
    {
        SYS_CALL_SuspendAllInterrupts();
        *value = Rte_Buffer_WinArbitrator_req_d_request;
        SYS_CALL_ResumeAllInterrupts();
    }

    return status;
}

/** ------ req_p */
Std_ReturnType Rte_Read_WinArbitratorType_WinArbitrator_req_p_request(/*OUT*/requestType * value) {
    Std_ReturnType status = RTE_E_OK;
    /* --- Unconnected */

    return status;
}

#define Rte_STOP_SEC_CODE
#include <Rte_MemMap.h>
#define Rte_START_SEC_CODE
#include <Rte_MemMap.h>

/** === WinControllerType ======================================================================= */
/** --- WinController -------------------------------------------------------------------- */

/** ------ cmd */

Std_ReturnType Rte_Write_WinControllerType_WinController_cmd_command(/*IN*/commandType value) {
    Std_ReturnType retVal = RTE_E_OK;

    /* --- Sender (SR7) */
    {
        SYS_CALL_SuspendAllInterrupts();
        Rte_Buffer_MotorDriver_cmd_command = value;

        SYS_CALL_ResumeAllInterrupts();
    }

    return retVal;
}

/** ------ endStop */
Std_ReturnType Rte_Read_WinControllerType_WinController_endStop_isPresent(/*OUT*/myBoolean * value) {
    Std_ReturnType status = RTE_E_OK;

    /* --- Receiver (SR9) */
    {
        SYS_CALL_SuspendAllInterrupts();
        *value = Rte_Buffer_WinController_endStop_isPresent;
        SYS_CALL_ResumeAllInterrupts();
    }

    return status;
}

/** ------ obstacle */
Std_ReturnType Rte_Read_WinControllerType_WinController_obstacle_isPresent(/*OUT*/myBoolean * value) {
    Std_ReturnType status = RTE_E_OK;

    /* --- Receiver (SR8) */
    {
        SYS_CALL_SuspendAllInterrupts();
        *value = Rte_Buffer_WinController_obstacle_isPresent;
        SYS_CALL_ResumeAllInterrupts();
    }

    return status;
}

/** ------ req */
Std_ReturnType Rte_Read_WinControllerType_WinController_req_request(/*OUT*/requestType * value) {
    Std_ReturnType status = RTE_E_OK;

    /* --- Receiver (SR6) */
    {
        SYS_CALL_SuspendAllInterrupts();
        *value = Rte_Buffer_WinController_req_request;
        SYS_CALL_ResumeAllInterrupts();
    }

    return status;
}

#define Rte_STOP_SEC_CODE
#include <Rte_MemMap.h>

#define Rte_START_SEC_CODE
#include <Rte_MemMap.h>
void Rte_Internal_Init_Buffers(void) {

}

#define Rte_STOP_SEC_CODE
#include <Rte_MemMap.h>
