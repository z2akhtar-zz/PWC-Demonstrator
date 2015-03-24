
/*
 * Generator version: 1.0.0
 * AUTOSAR version:   4.0.3
 */

#ifndef IOHWAB_DCM_H_
#define IOHWAB_DCM_H_

#include "Dcm_Types.h"

/* Digital signal: MOTOR_1 */
Std_ReturnType IoHwAb_Dcm_MOTOR_1(uint8 action, uint8 *value);
Std_ReturnType IoHwAb_Dcm_Read_MOTOR_1(uint8 *value);

/* Digital signal: EndStop */
Std_ReturnType IoHwAb_Dcm_EndStop(uint8 action, uint8 *value);
Std_ReturnType IoHwAb_Dcm_Read_EndStop(uint8 *value);

/* Digital signal: Obstacle */
Std_ReturnType IoHwAb_Dcm_Obstacle(uint8 action, uint8 *value);
Std_ReturnType IoHwAb_Dcm_Read_Obstacle(uint8 *value);

/* Digital signal: MOTOR_2 */
Std_ReturnType IoHwAb_Dcm_MOTOR_2(uint8 action, uint8 *value);
Std_ReturnType IoHwAb_Dcm_Read_MOTOR_2(uint8 *value);




#endif /* IOHWAB_DCM_H_ */

