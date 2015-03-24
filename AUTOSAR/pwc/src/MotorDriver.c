#include "PWC.h"
#include "Rte_MotorDriverType.h"

void MotorDriverRunnable(void)
{
	commandType command;
	command = Rte_IRead_MotorDriverRunnable_cmd_command();

	switch (command)
	{
	case command_neutral:
		Rte_Call_RunMotor_1_Write(IOHWAB_LOW);
		Rte_Call_RunMotor_2_Write(IOHWAB_LOW);
	//	Dio_WriteChannel(DioConf_DioChannel_LED_1, IOHWAB_LOW);
	//	Dio_WriteChannel(DioConf_DioChannel_LED_2, IOHWAB_LOW);
	//	Dio_WriteChannel(DioConf_DioChannel_LED_3, IOHWAB_LOW);
	//	Dio_WriteChannel(DioConf_DioChannel_LED_4, IOHWAB_LOW);
		break;
	case command_down:
		Rte_Call_RunMotor_1_Write(IOHWAB_HIGH);
		Rte_Call_RunMotor_2_Write(IOHWAB_LOW);
	//	Dio_WriteChannel(DioConf_DioChannel_LED_3, IOHWAB_HIGH);
		break;
	case command_up:
		Rte_Call_RunMotor_1_Write(IOHWAB_LOW);
		Rte_Call_RunMotor_2_Write(IOHWAB_HIGH);
	//	Dio_WriteChannel(DioConf_DioChannel_LED_4, IOHWAB_HIGH);
		break;
	}
}
