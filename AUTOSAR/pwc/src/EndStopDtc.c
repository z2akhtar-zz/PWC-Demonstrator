#include "PWC.h"
#include "Rte_EndStopDtcType.h"

void EndStopDtcRunnable(void)
{
	IoHwAb_LevelType value;
	uint8 status;
	Rte_Call_isEndStop_Read(&value, &status);
/*
	if (value)
		Dio_WriteChannel(DioConf_DioChannel_LED_4, 1);
	else
		Dio_WriteChannel(DioConf_DioChannel_LED_4, 0);
*/
	if (value)
		Rte_IWrite_EndStopDtcRunnable_endStop_isPresent(IOHWAB_LOW);
	else
		Rte_IWrite_EndStopDtcRunnable_endStop_isPresent(IOHWAB_HIGH);
}
