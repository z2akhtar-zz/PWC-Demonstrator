
/*
 * Generator version: 1.0.0
 * AUTOSAR version:   4.0.3
 */

#include "IoHwAb.h"
#include "IoHwAb_Internal.h"
#include "IoHwAb_Digital.h"
#include "IoHwAb_Dcm.h"

#if defined(USE_DIO)
#include "Dio.h"
#else
#error "DIO Module is needed by IOHWAB"
#endif

#if defined(USE_DET) 
#include "Det.h"
#else
#error Need to add DET module when ArcIoHwAbDevErrorDetect is enabled
#endif 


#define IS_VALID_DIO_LEVEL(_x) ((STD_LOW == (_x)) || (STD_HIGH == (_x)))


/* Signals states for I/O-control */
/* Digital signal: MOTOR_1 */
boolean IoHwAb_MOTOR_1_Locked = FALSE;
IoHwAb_LevelType IoHwAb_MOTOR_1_Saved = IOHWAB_LOW;
const IoHwAb_LevelType IoHwAb_MOTOR_1_Default = IOHWAB_LOW;

/* Digital signal: EndStop */
boolean IoHwAb_EndStop_Locked = FALSE;
IoHwAb_LevelType IoHwAb_EndStop_Saved = IOHWAB_LOW;
const IoHwAb_LevelType IoHwAb_EndStop_Default = IOHWAB_HIGH;

/* Digital signal: Obstacle */
boolean IoHwAb_Obstacle_Locked = FALSE;
IoHwAb_LevelType IoHwAb_Obstacle_Saved = IOHWAB_LOW;
const IoHwAb_LevelType IoHwAb_Obstacle_Default = IOHWAB_HIGH;

/* Digital signal: MOTOR_2 */
boolean IoHwAb_MOTOR_2_Locked = FALSE;
IoHwAb_LevelType IoHwAb_MOTOR_2_Saved = IOHWAB_LOW;
const IoHwAb_LevelType IoHwAb_MOTOR_2_Default = IOHWAB_LOW;

Std_ReturnType IoHwAb_Digital_Write_MOTOR_1(IoHwAb_LevelType newValue)
{
	IOHWAB_VALIDATE_RETURN(IS_VALID_DIO_LEVEL((Dio_LevelType)newValue), IOHWAB_DIGITAL_WRITE_ID, IOHWAB_E_PARAM_LEVEL, E_NOT_OK);
	Dio_LevelType setLevel;
	if( TRUE == IoHwAb_MOTOR_1_Locked ) {
		setLevel = IoHwAb_MOTOR_1_Saved;
	} else {
		setLevel = newValue;
	}
	IoHwAb_MOTOR_1_Saved = setLevel;
	/* @req ARCIOHWAB004 */
	Dio_WriteChannel(DioConf_DioChannel_LED_1, setLevel);
	return E_OK;
}

Std_ReturnType IoHwAb_Digital_Write_MOTOR_2(IoHwAb_LevelType newValue)
{
	IOHWAB_VALIDATE_RETURN(IS_VALID_DIO_LEVEL((Dio_LevelType)newValue), IOHWAB_DIGITAL_WRITE_ID, IOHWAB_E_PARAM_LEVEL, E_NOT_OK);
	Dio_LevelType setLevel;
	if( TRUE == IoHwAb_MOTOR_2_Locked ) {
		setLevel = IoHwAb_MOTOR_2_Saved;
	} else {
		setLevel = newValue;
	}
	IoHwAb_MOTOR_2_Saved = setLevel;
	/* @req ARCIOHWAB004 */
	Dio_WriteChannel(DioConf_DioChannel_LED_2, setLevel);
	return E_OK;
}

/* @req ARCIOHWAB011 */
Std_ReturnType IoHwAb_Digital_Read_MOTOR_1(IoHwAb_LevelType *value, IoHwAb_StatusType *status)
{
	IOHWAB_VALIDATE_RETURN((NULL != value), IOHWAB_DIGITAL_READ_ID, IOHWAB_E_PARAM_PTR, E_NOT_OK);
	IOHWAB_VALIDATE_RETURN((NULL != status), IOHWAB_DIGITAL_READ_ID, IOHWAB_E_PARAM_PTR, E_NOT_OK);
	*value = Dio_ReadChannel(DioConf_DioChannel_LED_1);
	status->quality = IOHWAB_GOOD;

	return E_OK;
}

/* @req ARCIOHWAB011 */
Std_ReturnType IoHwAb_Digital_Read_EndStop(IoHwAb_LevelType *value, IoHwAb_StatusType *status)
{
	IOHWAB_VALIDATE_RETURN((NULL != value), IOHWAB_DIGITAL_READ_ID, IOHWAB_E_PARAM_PTR, E_NOT_OK);
	IOHWAB_VALIDATE_RETURN((NULL != status), IOHWAB_DIGITAL_READ_ID, IOHWAB_E_PARAM_PTR, E_NOT_OK);
	if( FALSE == IoHwAb_EndStop_Locked ) {
		*value = Dio_ReadChannel(DioConf_DioChannel_Key);
	} else {
		*value = IoHwAb_EndStop_Saved;
	}
	status->quality = IOHWAB_GOOD;

	return E_OK;
}

/* @req ARCIOHWAB011 */
Std_ReturnType IoHwAb_Digital_Read_Obstacle(IoHwAb_LevelType *value, IoHwAb_StatusType *status)
{
	IOHWAB_VALIDATE_RETURN((NULL != value), IOHWAB_DIGITAL_READ_ID, IOHWAB_E_PARAM_PTR, E_NOT_OK);
	IOHWAB_VALIDATE_RETURN((NULL != status), IOHWAB_DIGITAL_READ_ID, IOHWAB_E_PARAM_PTR, E_NOT_OK);
	if( FALSE == IoHwAb_Obstacle_Locked ) {
		*value = Dio_ReadChannel(DioConf_DioChannel_Tamper);
	} else {
		*value = IoHwAb_Obstacle_Saved;
	}
	status->quality = IOHWAB_GOOD;

	return E_OK;
}

/* @req ARCIOHWAB011 */
Std_ReturnType IoHwAb_Digital_Read_MOTOR_2(IoHwAb_LevelType *value, IoHwAb_StatusType *status)
{
	IOHWAB_VALIDATE_RETURN((NULL != value), IOHWAB_DIGITAL_READ_ID, IOHWAB_E_PARAM_PTR, E_NOT_OK);
	IOHWAB_VALIDATE_RETURN((NULL != status), IOHWAB_DIGITAL_READ_ID, IOHWAB_E_PARAM_PTR, E_NOT_OK);
	*value = Dio_ReadChannel(DioConf_DioChannel_LED_2);
	status->quality = IOHWAB_GOOD;

	return E_OK;
}

/* Exported functions */
/* Digital signal: MOTOR_1 */
/* @req ARCIOHWAB003 */
Std_ReturnType IoHwAb_Dcm_MOTOR_1(uint8 action, uint8* value)
{
	Std_ReturnType ret = E_OK;
	imask_t state;
	IoHwAb_LockSave(state);
	boolean tempLock = IoHwAb_MOTOR_1_Locked;
	switch(action) {
	case IOHWAB_RETURNCONTROLTOECU:
		IoHwAb_MOTOR_1_Locked = FALSE;
		break;
	case IOHWAB_RESETTODEFAULT:
		IoHwAb_MOTOR_1_Locked = FALSE;
		if(E_OK != IoHwAb_Digital_Write_MOTOR_1(IoHwAb_MOTOR_1_Default)) {
			IoHwAb_MOTOR_1_Locked = tempLock;
			ret = E_NOT_OK;
		} else {
			IoHwAb_MOTOR_1_Saved = IoHwAb_MOTOR_1_Default;
			IoHwAb_MOTOR_1_Locked = TRUE;
		}
		break;
	case IOHWAB_FREEZECURRENTSTATE:
		IoHwAb_MOTOR_1_Locked = TRUE;
		break;
	case IOHWAB_SHORTTERMADJUST:
		{
			IoHwAb_LevelType level = *((IoHwAb_LevelType*)value);
			if(IS_VALID_DIO_LEVEL((Dio_LevelType)level)) {
				IoHwAb_MOTOR_1_Locked = FALSE;
				if(E_OK != IoHwAb_Digital_Write_MOTOR_1(level)) {
					IoHwAb_MOTOR_1_Locked = tempLock;
					ret = E_NOT_OK;
				} else {
					IoHwAb_MOTOR_1_Saved = level;
					IoHwAb_MOTOR_1_Locked = TRUE;
				}
			} else {
				IOHWAB_DET_REPORT_ERROR(IOHWAB_DIGITAL_IO_CONTROL_ID, IOHWAB_E_PARAM_LEVEL);
				ret = E_NOT_OK;
			}
		}
		break;
	default:
		IOHWAB_DET_REPORT_ERROR(IOHWAB_DIGITAL_IO_CONTROL_ID, IOHWAB_E_PARAM_ACTION);
		ret = E_NOT_OK;
		break;
	}
	IoHwAb_LockRestore(state);
	return ret;
}


Std_ReturnType IoHwAb_Dcm_Read_MOTOR_1(uint8* value)
{
	Std_ReturnType ret;
	IoHwAb_StatusType status;
	imask_t state;
	IoHwAb_LockSave(state);
	boolean tempLock = IoHwAb_MOTOR_1_Locked;
	IoHwAb_MOTOR_1_Locked = FALSE;
	ret = IoHwAb_Digital_Read_MOTOR_1((IoHwAb_LevelType*)value, &status);
	IoHwAb_MOTOR_1_Locked = tempLock;
	IoHwAb_LockRestore(state);
	return ret;
}

	


/* Digital signal: EndStop */
/* @req ARCIOHWAB003 */
Std_ReturnType IoHwAb_Dcm_EndStop(uint8 action, uint8* value)
{
	Std_ReturnType ret = E_OK;
	IoHwAb_StatusType status;
	imask_t state;
	IoHwAb_LockSave(state);
	boolean tempLock = IoHwAb_EndStop_Locked;
	switch(action) {
	case IOHWAB_RETURNCONTROLTOECU:
		IoHwAb_EndStop_Locked = FALSE;
		break;
	case IOHWAB_RESETTODEFAULT:
		IoHwAb_EndStop_Saved = IoHwAb_EndStop_Default;
		IoHwAb_EndStop_Locked = TRUE;
		break;
	case IOHWAB_FREEZECURRENTSTATE:
		IoHwAb_EndStop_Locked = FALSE;
		if(E_OK != IoHwAb_Digital_Read_EndStop(&IoHwAb_EndStop_Saved, &status)) {
			IoHwAb_EndStop_Locked = tempLock;
			ret = E_NOT_OK;
		} else {
			IoHwAb_EndStop_Locked = TRUE;
		}
		break;
	case IOHWAB_SHORTTERMADJUST:
		{
			IoHwAb_LevelType level = *((IoHwAb_LevelType*)value);
			if(IS_VALID_DIO_LEVEL((Dio_LevelType)level)) {
				IoHwAb_EndStop_Saved = level;
				IoHwAb_EndStop_Locked = TRUE;
			} else {
				IOHWAB_DET_REPORT_ERROR(IOHWAB_DIGITAL_IO_CONTROL_ID, IOHWAB_E_PARAM_LEVEL);
				ret = E_NOT_OK;
			}
		}
		break;
	default:
		IOHWAB_DET_REPORT_ERROR(IOHWAB_DIGITAL_IO_CONTROL_ID, IOHWAB_E_PARAM_ACTION);
		ret = E_NOT_OK;
		break;
	}
	IoHwAb_LockRestore(state);
	return ret;
}


Std_ReturnType IoHwAb_Dcm_Read_EndStop(uint8* value)
{
	Std_ReturnType ret;
	IoHwAb_StatusType status;
	imask_t state;
	IoHwAb_LockSave(state);
	boolean tempLock = IoHwAb_EndStop_Locked;
	IoHwAb_EndStop_Locked = FALSE;
	ret = IoHwAb_Digital_Read_EndStop((IoHwAb_LevelType*)value, &status);
	IoHwAb_EndStop_Locked = tempLock;
	IoHwAb_LockRestore(state);
	return ret;
}

	


/* Digital signal: Obstacle */
/* @req ARCIOHWAB003 */
Std_ReturnType IoHwAb_Dcm_Obstacle(uint8 action, uint8* value)
{
	Std_ReturnType ret = E_OK;
	IoHwAb_StatusType status;
	imask_t state;
	IoHwAb_LockSave(state);
	boolean tempLock = IoHwAb_Obstacle_Locked;
	switch(action) {
	case IOHWAB_RETURNCONTROLTOECU:
		IoHwAb_Obstacle_Locked = FALSE;
		break;
	case IOHWAB_RESETTODEFAULT:
		IoHwAb_Obstacle_Saved = IoHwAb_Obstacle_Default;
		IoHwAb_Obstacle_Locked = TRUE;
		break;
	case IOHWAB_FREEZECURRENTSTATE:
		IoHwAb_Obstacle_Locked = FALSE;
		if(E_OK != IoHwAb_Digital_Read_Obstacle(&IoHwAb_Obstacle_Saved, &status)) {
			IoHwAb_Obstacle_Locked = tempLock;
			ret = E_NOT_OK;
		} else {
			IoHwAb_Obstacle_Locked = TRUE;
		}
		break;
	case IOHWAB_SHORTTERMADJUST:
		{
			IoHwAb_LevelType level = *((IoHwAb_LevelType*)value);
			if(IS_VALID_DIO_LEVEL((Dio_LevelType)level)) {
				IoHwAb_Obstacle_Saved = level;
				IoHwAb_Obstacle_Locked = TRUE;
			} else {
				IOHWAB_DET_REPORT_ERROR(IOHWAB_DIGITAL_IO_CONTROL_ID, IOHWAB_E_PARAM_LEVEL);
				ret = E_NOT_OK;
			}
		}
		break;
	default:
		IOHWAB_DET_REPORT_ERROR(IOHWAB_DIGITAL_IO_CONTROL_ID, IOHWAB_E_PARAM_ACTION);
		ret = E_NOT_OK;
		break;
	}
	IoHwAb_LockRestore(state);
	return ret;
}


Std_ReturnType IoHwAb_Dcm_Read_Obstacle(uint8* value)
{
	Std_ReturnType ret;
	IoHwAb_StatusType status;
	imask_t state;
	IoHwAb_LockSave(state);
	boolean tempLock = IoHwAb_Obstacle_Locked;
	IoHwAb_Obstacle_Locked = FALSE;
	ret = IoHwAb_Digital_Read_Obstacle((IoHwAb_LevelType*)value, &status);
	IoHwAb_Obstacle_Locked = tempLock;
	IoHwAb_LockRestore(state);
	return ret;
}

	


/* Digital signal: MOTOR_2 */
/* @req ARCIOHWAB003 */
Std_ReturnType IoHwAb_Dcm_MOTOR_2(uint8 action, uint8* value)
{
	Std_ReturnType ret = E_OK;
	imask_t state;
	IoHwAb_LockSave(state);
	boolean tempLock = IoHwAb_MOTOR_2_Locked;
	switch(action) {
	case IOHWAB_RETURNCONTROLTOECU:
		IoHwAb_MOTOR_2_Locked = FALSE;
		break;
	case IOHWAB_RESETTODEFAULT:
		IoHwAb_MOTOR_2_Locked = FALSE;
		if(E_OK != IoHwAb_Digital_Write_MOTOR_2(IoHwAb_MOTOR_2_Default)) {
			IoHwAb_MOTOR_2_Locked = tempLock;
			ret = E_NOT_OK;
		} else {
			IoHwAb_MOTOR_2_Saved = IoHwAb_MOTOR_2_Default;
			IoHwAb_MOTOR_2_Locked = TRUE;
		}
		break;
	case IOHWAB_FREEZECURRENTSTATE:
		IoHwAb_MOTOR_2_Locked = TRUE;
		break;
	case IOHWAB_SHORTTERMADJUST:
		{
			IoHwAb_LevelType level = *((IoHwAb_LevelType*)value);
			if(IS_VALID_DIO_LEVEL((Dio_LevelType)level)) {
				IoHwAb_MOTOR_2_Locked = FALSE;
				if(E_OK != IoHwAb_Digital_Write_MOTOR_2(level)) {
					IoHwAb_MOTOR_2_Locked = tempLock;
					ret = E_NOT_OK;
				} else {
					IoHwAb_MOTOR_2_Saved = level;
					IoHwAb_MOTOR_2_Locked = TRUE;
				}
			} else {
				IOHWAB_DET_REPORT_ERROR(IOHWAB_DIGITAL_IO_CONTROL_ID, IOHWAB_E_PARAM_LEVEL);
				ret = E_NOT_OK;
			}
		}
		break;
	default:
		IOHWAB_DET_REPORT_ERROR(IOHWAB_DIGITAL_IO_CONTROL_ID, IOHWAB_E_PARAM_ACTION);
		ret = E_NOT_OK;
		break;
	}
	IoHwAb_LockRestore(state);
	return ret;
}


Std_ReturnType IoHwAb_Dcm_Read_MOTOR_2(uint8* value)
{
	Std_ReturnType ret;
	IoHwAb_StatusType status;
	imask_t state;
	IoHwAb_LockSave(state);
	boolean tempLock = IoHwAb_MOTOR_2_Locked;
	IoHwAb_MOTOR_2_Locked = FALSE;
	ret = IoHwAb_Digital_Read_MOTOR_2((IoHwAb_LevelType*)value, &status);
	IoHwAb_MOTOR_2_Locked = tempLock;
	IoHwAb_LockRestore(state);
	return ret;
}

	


/* @req ARCIOHWAB001 */
Std_ReturnType IoHwAb_Digital_Write(IoHwAb_SignalType signal, IoHwAb_LevelType newValue)
{
	Std_ReturnType ret = E_NOT_OK;

	switch( signal ) {
	case IOHWAB_SIGNAL_MOTOR_1:
		ret = IoHwAb_Digital_Write_MOTOR_1(newValue);
		break;
	case IOHWAB_SIGNAL_MOTOR_2:
		ret = IoHwAb_Digital_Write_MOTOR_2(newValue);
		break;
	default:
		IOHWAB_DET_REPORT_ERROR(IOHWAB_DIGITAL_WRITE_ID, IOHWAB_E_PARAM_SIGNAL);
		break;
	}
	return ret;
}

/* @req ARCIOHWAB001 */
Std_ReturnType IoHwAb_Digital_Read(IoHwAb_SignalType signal, IoHwAb_LevelType *value, IoHwAb_StatusType *status)
{
	Std_ReturnType ret = E_NOT_OK;

	switch( signal ) {
	case IOHWAB_SIGNAL_ENDSTOP:
		ret = IoHwAb_Digital_Read_EndStop(value, status);
		break;
	case IOHWAB_SIGNAL_OBSTACLE:
		ret = IoHwAb_Digital_Read_Obstacle(value, status);
		break;
	default:
		IOHWAB_DET_REPORT_ERROR(IOHWAB_DIGITAL_READ_ID, IOHWAB_E_PARAM_SIGNAL);
		break;
	}
	return ret;
}

