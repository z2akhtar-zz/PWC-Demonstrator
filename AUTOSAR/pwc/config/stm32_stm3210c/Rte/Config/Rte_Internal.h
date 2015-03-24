#ifndef RTE_INTERNAL_H_
#define RTE_INTERNAL_H_

#include <Rte_DataHandleType.h>

void Rte_Internal_Init_Buffers(void);

/** --- PORT STATUS TYPES ---------------------------------------------------------- */

/** --- SERVER REQUEST TYPES ---------------------------------------------------------------- */

/** --- SERVER RESPONSE TYPES --------------------------------------------------------------- */

typedef enum {
    RTE_NO_REQUEST_PENDING = 0, RTE_REQUEST_PENDING = 1, RTE_RESPONSE_RECEIVED = 2
} Rte_ResponseStatusType;

/** --- MODE MACHINE TYPES ------------------------------------------------------------------ */

/** --- MODE MACHINE TRANSITION DEFINES ----------------------------------------------------- */

/** --- MODE MACHINE VALUE DEFINES ---------------------------------------------------------- */

/** --- EXCLUSIVE AREA TYPES ---------------------------------------------------------------- */
typedef struct {
    boolean entered;
} ExclusiveAreaType;

/** --- EXPORTED FUNCTIONS ----------------------------------------------------------------- */
/** === EndStopDtcType ======================================================================= */
/** --- EndStopDtc -------------------------------------------------------------------- */

/** ------ endStop */
Std_ReturnType Rte_Write_EndStopDtcType_EndStopDtc_endStop_isPresent(/*IN*/myBoolean data);

/** ------ isEndStop */
Std_ReturnType Rte_Call_EndStopDtcType_EndStopDtc_isEndStop_Read(/*OUT*/DigitalLevel * Level, /*OUT*/SignalQuality * Quality);

/** === IoHwAb ======================================================================= */
/** --- ioHwAb -------------------------------------------------------------------- */

/** ------ Digital_DigitalSignal_EndStop */
Std_ReturnType Rte_Call_IoHwAb_ioHwAb_Digital_DigitalSignal_EndStop_Read(/*OUT*/DigitalLevel * Level, /*OUT*/SignalQuality * Quality);

/** ------ Digital_DigitalSignal_LED1 */
Std_ReturnType Rte_Call_IoHwAb_ioHwAb_Digital_DigitalSignal_LED1_Write(/*IN*/DigitalLevel Level);

/** ------ Digital_DigitalSignal_LED2 */
Std_ReturnType Rte_Call_IoHwAb_ioHwAb_Digital_DigitalSignal_LED2_Write(/*IN*/DigitalLevel Level);

/** ------ Digital_DigitalSignal_Obstacle */
Std_ReturnType Rte_Call_IoHwAb_ioHwAb_Digital_DigitalSignal_Obstacle_Read(/*OUT*/DigitalLevel * Level, /*OUT*/SignalQuality * Quality);

/** === MotorDriverType ======================================================================= */
/** --- MotorDriver -------------------------------------------------------------------- */

/** ------ RunMotor_1 */
Std_ReturnType Rte_Call_MotorDriverType_MotorDriver_RunMotor_1_Write(/*IN*/DigitalLevel Level);

/** ------ RunMotor_2 */
Std_ReturnType Rte_Call_MotorDriverType_MotorDriver_RunMotor_2_Write(/*IN*/DigitalLevel Level);

/** ------ cmd */
Std_ReturnType Rte_Read_MotorDriverType_MotorDriver_cmd_command(/*OUT*/commandType * data);

/** === ObstacleDtcType ======================================================================= */
/** --- ObstacleDtc -------------------------------------------------------------------- */

/** ------ isObstacle */
Std_ReturnType Rte_Call_ObstacleDtcType_ObstacleDtc_isObstacle_Read(/*OUT*/DigitalLevel * Level, /*OUT*/SignalQuality * Quality);

/** ------ obstacle */
Std_ReturnType Rte_Write_ObstacleDtcType_ObstacleDtc_obstacle_isPresent(/*IN*/myBoolean data);

/** === SwitchType ======================================================================= */
/** --- Switch -------------------------------------------------------------------- */

/** ------ req */
Std_ReturnType Rte_Write_SwitchType_Switch_req_request(/*IN*/requestType data);

/** === WinArbitratorType ======================================================================= */
/** --- WinArbitrator -------------------------------------------------------------------- */

/** ------ req_a */
Std_ReturnType Rte_Write_WinArbitratorType_WinArbitrator_req_a_request(/*IN*/requestType data);

/** ------ req_d */
Std_ReturnType Rte_Read_WinArbitratorType_WinArbitrator_req_d_request(/*OUT*/requestType * data);

/** ------ req_p */
Std_ReturnType Rte_Read_WinArbitratorType_WinArbitrator_req_p_request(/*OUT*/requestType * data);

/** === WinControllerType ======================================================================= */
/** --- WinController -------------------------------------------------------------------- */

/** ------ cmd */
Std_ReturnType Rte_Write_WinControllerType_WinController_cmd_command(/*IN*/commandType data);

/** ------ endStop */
Std_ReturnType Rte_Read_WinControllerType_WinController_endStop_isPresent(/*OUT*/myBoolean * data);

/** ------ obstacle */
Std_ReturnType Rte_Read_WinControllerType_WinController_obstacle_isPresent(/*OUT*/myBoolean * data);

/** ------ req */
Std_ReturnType Rte_Read_WinControllerType_WinController_req_request(/*OUT*/requestType * data);

#endif /* RTE_INTERNAL_H_ */

