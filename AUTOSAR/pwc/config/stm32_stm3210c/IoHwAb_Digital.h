
/*
 * Generator version: 1.0.0
 * AUTOSAR version:   4.0.3
 */

#ifndef IOHWAB_DIGITAL_H_
#define IOHWAB_DIGITAL_H_

#include "Std_Types.h"

#define IOHWAB_SIGNAL_MOTOR_1 ((IoHwAb_SignalType)0)
#define IOHWAB_SIGNAL_ENDSTOP ((IoHwAb_SignalType)1)
#define IOHWAB_SIGNAL_OBSTACLE ((IoHwAb_SignalType)2)
#define IOHWAB_SIGNAL_MOTOR_2 ((IoHwAb_SignalType)3)

Std_ReturnType IoHwAb_Digital_Write_MOTOR_1(IoHwAb_LevelType newValue);
Std_ReturnType IoHwAb_Digital_Write_MOTOR_2(IoHwAb_LevelType newValue);
Std_ReturnType IoHwAb_Digital_Read_EndStop(IoHwAb_LevelType *value, IoHwAb_StatusType *status);
Std_ReturnType IoHwAb_Digital_Read_Obstacle(IoHwAb_LevelType *value, IoHwAb_StatusType *status);

Std_ReturnType IoHwAb_Digital_Write(IoHwAb_SignalType signal, IoHwAb_LevelType newValue);
Std_ReturnType IoHwAb_Digital_Read(IoHwAb_SignalType signal, IoHwAb_LevelType *value, IoHwAb_StatusType *status);


#endif /* IOHWAB_DIGITAL_H_ */


