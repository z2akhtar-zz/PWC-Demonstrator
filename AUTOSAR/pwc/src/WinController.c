#include "PWC.h"
#include "Rte_WinControllerType.h"

/* Named constants for Chart: '<Root>/WinController' */
#define power_window_IN_NO_ACTIVE_CHILD ((uint8_T)0U)
#define power_window_c_IN_movingDownBsc ((uint8_T)1U)
#define power_window_c_IN_partiallyOpen ((uint8_T)3U)
#define power_window_con_IN_fullyClosed ((uint8_T)1U)
#define power_window_con_IN_movingDownX ((uint8_T)2U)
#define power_window_con_IN_movingUpBsc ((uint8_T)1U)
#define power_window_cont_IN_movingDown ((uint8_T)3U)
#define power_window_cont_IN_retracting ((uint8_T)5U)
#define power_window_cont_IN_unknownPos ((uint8_T)4U)
#define power_window_contr_IN_fullyOpen ((uint8_T)2U)
#define power_window_contr_IN_movingUpX ((uint8_T)2U)
#define power_window_contr_IN_winStates ((uint8_T)1U)
#define power_window_contro_IN_coolDown ((uint8_T)2U)
#define power_window_contro_IN_movingUp ((uint8_T)4U)
#define power_window_control_IN_Stopped ((uint8_T)1U)
#define power_window_control_IN_mux_ppo ((uint8_T)2U)
#define power_window_controll_IN_mux_pp ((uint8_T)1U)

/* Block states (auto storage) */
DW_PWC_T PWC_DW = {0};

/* Forward declaration for local functions */
static void power_window_controll_winStates(const uint8_T *rtu_req, const
  boolean_T *rtu_endStop, const boolean_T *rtu_obstacle, uint8_T *rty_cmd,
  DW_power_window_controller_f_T *localDW);

// Implementation code obtained from Runnable_Step in PWC.c
void WinControllerRunnable(void)
{
	requestType request;
	commandType command;
	myBoolean endstop_tmp;
	myBoolean obstacle_tmp;

    request = Rte_IRead_WinControllerStepRunnable_req_request();

	/* Inport: '<Root>/obstacle' */
	obstacle_tmp = Rte_IRead_WinControllerStepRunnable_obstacle_isPresent();

	/* Inport: '<Root>/endstop' */
	endstop_tmp = Rte_IRead_WinControllerStepRunnable_endStop_isPresent();

	/* Truth Table: '<Root>/WinArbitrator' */

	/* ModelReference: '<Root>/WinController' */

	power_window_controller(&request, &endstop_tmp, &obstacle_tmp,
			&command, &(PWC_DW.WinController_DWORK1.rtdw));

	/* Outport: '<Root>/cmd' */
	Rte_IWrite_WinControllerStepRunnable_cmd_command(command);
}

/* Output and update for referenced model: 'power_window_controller' */
void power_window_controller(const uint8_T *rtu_req, const boolean_T
		*rtu_endStop, const boolean_T *rtu_obstacle, uint8_T *rty_cmd,
		DW_power_window_controller_f_T *localDW)
{
	/* Chart: '<Root>/WinController' */
	if (localDW->temporalCounter_i1 < 255U) {
		localDW->temporalCounter_i1++;
	}

	/* Gateway: WinController */
	/* During: WinController */

	if (localDW->is_active_c3_power_window_contr == 0U) {
		/* Entry: WinController */
		localDW->is_active_c3_power_window_contr = 1U;

		/* Entry Internal: WinController */
		localDW->is_c3_power_window_controller = power_window_contr_IN_winStates;

		/* Entry Internal 'winStates': '<S1>:5' */
		/* Transition: '<S1>:144' */
		localDW->is_winStates = power_window_control_IN_Stopped;

		/* Entry 'Stopped': '<S1>:3' */
		*rty_cmd = 0U;

		/* Entry Internal 'Stopped': '<S1>:3' */
		/* Transition: '<S1>:58' */
		localDW->is_Stopped = power_window_cont_IN_unknownPos;

	} else {
		power_window_controll_winStates(rtu_req, rtu_endStop, rtu_obstacle, rty_cmd,
				localDW);
	}

	/* End of Chart: '<Root>/WinController' */
}

/* Function for Chart: '<Root>/WinController' */
static void power_window_controll_winStates(const uint8_T *rtu_req, const
		boolean_T *rtu_endStop, const boolean_T *rtu_obstacle, uint8_T *rty_cmd,
		DW_power_window_controller_f_T *localDW)
{
	/* During 'winStates': '<S1>:5' */
	switch (localDW->is_winStates) {

	case power_window_control_IN_Stopped:

		Dio_WriteChannel(DioConf_DioChannel_LED_4, IOHWAB_LOW);
    	Dio_WriteChannel(DioConf_DioChannel_LED_3, IOHWAB_LOW);

		/* During 'Stopped': '<S1>:3' */
		switch (localDW->is_Stopped) {
		case power_window_con_IN_fullyClosed:
			Dio_WriteChannel(DioConf_DioChannel_LED_1, IOHWAB_HIGH);
			Dio_WriteChannel(DioConf_DioChannel_LED_2, IOHWAB_HIGH);
			/* Chart: '<Root>/WinController' */
			/* During 'fullyClosed': '<S1>:25' */
			if (*rtu_req == 1) {
				/* Transition: '<S1>:101' */
				/* Transition: '<S1>:124' */
				localDW->is_Stopped = power_window_IN_NO_ACTIVE_CHILD;
				localDW->is_winStates = power_window_cont_IN_movingDown;
				localDW->temporalCounter_i1 = 0U;

				/* Entry 'movingDown': '<S1>:64' */
				*rty_cmd = 1U;

				/* Entry Internal 'movingDown': '<S1>:64' */
				/* Transition: '<S1>:135' */
				localDW->is_movingDown = power_window_c_IN_movingDownBsc;
			} else {
				if (*rtu_req == 3) {
					/* Transition: '<S1>:136' */
					/* Transition: '<S1>:74' */
					localDW->is_Stopped = power_window_IN_NO_ACTIVE_CHILD;
					localDW->is_winStates = power_window_cont_IN_movingDown;
					localDW->temporalCounter_i1 = 0U;

					/* Entry 'movingDown': '<S1>:64' */
					*rty_cmd = 1U;
					localDW->is_movingDown = power_window_con_IN_movingDownX;
				}
			}
			break;

		case power_window_contr_IN_fullyOpen:
			Dio_WriteChannel(DioConf_DioChannel_LED_2, IOHWAB_LOW);
			Dio_WriteChannel(DioConf_DioChannel_LED_1, IOHWAB_LOW);
			/* Chart: '<Root>/WinController' */
			/* During 'fullyOpen': '<S1>:24' */
			if (*rtu_req == 4) {
				/* Transition: '<S1>:110' */
				/* Transition: '<S1>:79' */
				localDW->is_Stopped = power_window_IN_NO_ACTIVE_CHILD;
				localDW->is_winStates = power_window_contro_IN_movingUp;
				localDW->temporalCounter_i1 = 0U;

				/* Entry 'movingUp': '<S1>:2' */
				*rty_cmd = 2U;
				localDW->is_movingUp = power_window_contr_IN_movingUpX;

				/* Entry Internal 'movingUpX': '<S1>:12' */
				/* Transition: '<S1>:20' */
				localDW->is_movingUpX = power_window_control_IN_mux_ppo;
			} else {
				if (*rtu_req == 2) {
					/* Transition: '<S1>:102' */
					/* Transition: '<S1>:129' */
					localDW->is_Stopped = power_window_IN_NO_ACTIVE_CHILD;
					localDW->is_winStates = power_window_contro_IN_movingUp;
					localDW->temporalCounter_i1 = 0U;

					/* Entry 'movingUp': '<S1>:2' */
					*rty_cmd = 2U;

					/* Entry Internal 'movingUp': '<S1>:2' */
					/* Transition: '<S1>:122' */
					localDW->is_movingUp = power_window_con_IN_movingUpBsc;
				}
			}
			break;

		case power_window_c_IN_partiallyOpen:
			Dio_WriteChannel(DioConf_DioChannel_LED_2, IOHWAB_LOW);
	    	Dio_WriteChannel(DioConf_DioChannel_LED_1, IOHWAB_HIGH);
			/* Chart: '<Root>/WinController' */
			/* During 'partiallyOpen': '<S1>:26' */
			/* Transition: '<S1>:109' */
			if (*rtu_req == 2) {
				/* Transition: '<S1>:90' */
				/* Transition: '<S1>:129' */
				localDW->is_Stopped = power_window_IN_NO_ACTIVE_CHILD;
				localDW->is_winStates = power_window_contro_IN_movingUp;
				localDW->temporalCounter_i1 = 0U;

				/* Entry 'movingUp': '<S1>:2' */
				*rty_cmd = 2U;

				/* Entry Internal 'movingUp': '<S1>:2' */
				/* Transition: '<S1>:122' */
				localDW->is_movingUp = power_window_con_IN_movingUpBsc;
			} else if (*rtu_req == 1) {
				/* Transition: '<S1>:88' */
				/* Transition: '<S1>:124' */
				localDW->is_Stopped = power_window_IN_NO_ACTIVE_CHILD;
				localDW->is_winStates = power_window_cont_IN_movingDown;
				localDW->temporalCounter_i1 = 0U;

				/* Entry 'movingDown': '<S1>:64' */
				*rty_cmd = 1U;

				/* Entry Internal 'movingDown': '<S1>:64' */
				/* Transition: '<S1>:135' */
				localDW->is_movingDown = power_window_c_IN_movingDownBsc;
			} else if (*rtu_req == 3) {
				/* Transition: '<S1>:107' */
				/* Transition: '<S1>:74' */
				localDW->is_Stopped = power_window_IN_NO_ACTIVE_CHILD;
				localDW->is_winStates = power_window_cont_IN_movingDown;
				localDW->temporalCounter_i1 = 0U;

				/* Entry 'movingDown': '<S1>:64' */
				*rty_cmd = 1U;
				localDW->is_movingDown = power_window_con_IN_movingDownX;
			} else {
				if (*rtu_req == 4) {
					/* Transition: '<S1>:108' */
					/* Transition: '<S1>:79' */
					localDW->is_Stopped = power_window_IN_NO_ACTIVE_CHILD;
					localDW->is_winStates = power_window_contro_IN_movingUp;
					localDW->temporalCounter_i1 = 0U;

					/* Entry 'movingUp': '<S1>:2' */
					*rty_cmd = 2U;
					localDW->is_movingUp = power_window_contr_IN_movingUpX;

					/* Entry Internal 'movingUpX': '<S1>:12' */
					/* Transition: '<S1>:20' */
					localDW->is_movingUpX = power_window_control_IN_mux_ppo;
				}
			}
			break;

		default:
			/* During 'unknownPos': '<S1>:57' */
			/* Transition: '<S1>:60' */
			localDW->is_Stopped = power_window_c_IN_partiallyOpen;
			break;
		}
		break;

		case power_window_contro_IN_coolDown:
			Dio_WriteChannel(DioConf_DioChannel_LED_3, IOHWAB_HIGH);
			Dio_WriteChannel(DioConf_DioChannel_LED_4, IOHWAB_HIGH);
			Dio_WriteChannel(DioConf_DioChannel_LED_2, IOHWAB_LOW);
			Dio_WriteChannel(DioConf_DioChannel_LED_1, IOHWAB_HIGH);
			/* Chart: '<Root>/WinController' */
			/* During 'coolDown': '<S1>:137' */
			if (*rtu_req == 0) {
				/* Transition: '<S1>:139' */
				localDW->is_winStates = power_window_control_IN_Stopped;

				/* Entry 'Stopped': '<S1>:3' */
				*rty_cmd = 0U;

				/* Entry Internal 'Stopped': '<S1>:3' */
				/* Transition: '<S1>:58' */
				localDW->is_Stopped = power_window_cont_IN_unknownPos;
			}
			break;

		case power_window_cont_IN_movingDown:
			Dio_WriteChannel(DioConf_DioChannel_LED_4, IOHWAB_LOW);
			Dio_WriteChannel(DioConf_DioChannel_LED_3, IOHWAB_HIGH);

			/* Chart: '<Root>/WinController' */
			/* During 'movingDown': '<S1>:64' */
			if (*rtu_endStop) {
				/* Transition: '<S1>:119' */
				/* Transition: '<S1>:133' */
				/* Exit Internal 'movingDown': '<S1>:64' */
				localDW->is_movingDown = power_window_IN_NO_ACTIVE_CHILD;
				localDW->is_winStates = power_window_control_IN_Stopped;

				/* Entry 'Stopped': '<S1>:3' */
				*rty_cmd = 0U;
				localDW->is_Stopped = power_window_contr_IN_fullyOpen;
			} else if (*rtu_req == 4) {
				/* Transition: '<S1>:81' */
				/* Transition: '<S1>:79' */
				/* Exit Internal 'movingDown': '<S1>:64' */
				localDW->is_movingDown = power_window_IN_NO_ACTIVE_CHILD;
				localDW->is_winStates = power_window_contro_IN_movingUp;
				localDW->temporalCounter_i1 = 0U;

				/* Entry 'movingUp': '<S1>:2' */
				*rty_cmd = 2U;
				localDW->is_movingUp = power_window_contr_IN_movingUpX;

				/* Entry Internal 'movingUpX': '<S1>:12' */
				/* Transition: '<S1>:20' */
				localDW->is_movingUpX = power_window_control_IN_mux_ppo;
			} else if (localDW->temporalCounter_i1 >= 200U) {
				/* Transition: '<S1>:140' */
				/* Exit Internal 'movingDown': '<S1>:64' */
				localDW->is_movingDown = power_window_IN_NO_ACTIVE_CHILD;
				localDW->is_winStates = power_window_contro_IN_coolDown;

				/* Entry 'coolDown': '<S1>:137' */
				*rty_cmd = 0U;
			} else if (*rtu_req == 2) {
				/* Transition: '<S1>:80' */
				/* Transition: '<S1>:76' */
				/* Exit Internal 'movingDown': '<S1>:64' */
				localDW->is_movingDown = power_window_IN_NO_ACTIVE_CHILD;
				localDW->is_winStates = power_window_contro_IN_movingUp;
				localDW->temporalCounter_i1 = 0U;

				/* Entry 'movingUp': '<S1>:2' */
				*rty_cmd = 2U;
				localDW->is_movingUp = power_window_con_IN_movingUpBsc;
			}

			else if (localDW->is_movingDown == power_window_c_IN_movingDownBsc) {
				Dio_WriteChannel(DioConf_DioChannel_LED_1, IOHWAB_HIGH);
				Dio_WriteChannel(DioConf_DioChannel_LED_2, IOHWAB_LOW);
				/* During 'movingDownBsc': '<S1>:65' */
				if (*rtu_req == 3) {
					/* Transition: '<S1>:66' */
					localDW->is_movingDown = power_window_con_IN_movingDownX;
				} else {
					if (*rtu_req == 0) {
						/* Transition: '<S1>:83' */
						localDW->is_movingDown = power_window_IN_NO_ACTIVE_CHILD;
						localDW->is_winStates = power_window_control_IN_Stopped;

						/* Entry 'Stopped': '<S1>:3' */
						*rty_cmd = 0U;
						localDW->is_Stopped = power_window_c_IN_partiallyOpen;
					}
				}
			}

			else {
				/* During 'movingDownX': '<S1>:67' */
				Dio_WriteChannel(DioConf_DioChannel_LED_2, IOHWAB_HIGH);
				Dio_WriteChannel(DioConf_DioChannel_LED_1, IOHWAB_LOW);
			}
			break;

		case power_window_contro_IN_movingUp:
			Dio_WriteChannel(DioConf_DioChannel_LED_4, IOHWAB_HIGH);
			Dio_WriteChannel(DioConf_DioChannel_LED_3, IOHWAB_LOW);
			/* Chart: '<Root>/WinController' */
			/* During 'movingUp': '<S1>:2' */
			if (*rtu_endStop == 1) {
				/* Transition: '<S1>:56' */
				/* Exit Internal 'movingUp': '<S1>:2' */
				localDW->is_movingUp = power_window_IN_NO_ACTIVE_CHILD;

				/* Exit Internal 'movingUpX': '<S1>:12' */
				localDW->is_movingUpX = power_window_IN_NO_ACTIVE_CHILD;
				localDW->is_winStates = power_window_control_IN_Stopped;

				/* Entry 'Stopped': '<S1>:3' */
				*rty_cmd = 0U;
				localDW->is_Stopped = power_window_con_IN_fullyClosed;
			} else if (localDW->temporalCounter_i1 >= 200U) {
				/* Transition: '<S1>:138' */
				/* Exit Internal 'movingUp': '<S1>:2' */
				localDW->is_movingUp = power_window_IN_NO_ACTIVE_CHILD;

				/* Exit Internal 'movingUpX': '<S1>:12' */
				localDW->is_movingUpX = power_window_IN_NO_ACTIVE_CHILD;
				localDW->is_winStates = power_window_contro_IN_coolDown;

				/* Entry 'coolDown': '<S1>:137' */
				*rty_cmd = 0U;
			} else if (*rtu_req == 1) {
				/* Transition: '<S1>:86' */
				/* Transition: '<S1>:78' */
				/* Exit Internal 'movingUp': '<S1>:2' */
				localDW->is_movingUp = power_window_IN_NO_ACTIVE_CHILD;

				/* Exit Internal 'movingUpX': '<S1>:12' */
				localDW->is_movingUpX = power_window_IN_NO_ACTIVE_CHILD;
				localDW->is_winStates = power_window_cont_IN_movingDown;
				localDW->temporalCounter_i1 = 0U;

				/* Entry 'movingDown': '<S1>:64' */
				*rty_cmd = 1U;
				localDW->is_movingDown = power_window_c_IN_movingDownBsc;
			} else if (*rtu_req == 3) {
				/* Transition: '<S1>:87' */
				/* Transition: '<S1>:74' */
				/* Exit Internal 'movingUp': '<S1>:2' */
				localDW->is_movingUp = power_window_IN_NO_ACTIVE_CHILD;

				/* Exit Internal 'movingUpX': '<S1>:12' */
				localDW->is_movingUpX = power_window_IN_NO_ACTIVE_CHILD;
				localDW->is_winStates = power_window_cont_IN_movingDown;
				localDW->temporalCounter_i1 = 0U;

				/* Entry 'movingDown': '<S1>:64' */
				*rty_cmd = 1U;
				localDW->is_movingDown = power_window_con_IN_movingDownX;
			}

			else if (localDW->is_movingUp == power_window_con_IN_movingUpBsc) {
				Dio_WriteChannel(DioConf_DioChannel_LED_1, IOHWAB_LOW);
				Dio_WriteChannel(DioConf_DioChannel_LED_2, IOHWAB_LOW);
				/* During 'movingUpBsc': '<S1>:31' */
				if (*rtu_req == 4) {
					/* Transition: '<S1>:61' */
					localDW->is_movingUp = power_window_contr_IN_movingUpX;

					/* Entry Internal 'movingUpX': '<S1>:12' */
					/* Transition: '<S1>:20' */
					localDW->is_movingUpX = power_window_control_IN_mux_ppo;
				} else {
					if (*rtu_req == 0) {
						/* Transition: '<S1>:84' */
						localDW->is_movingUp = power_window_IN_NO_ACTIVE_CHILD;
						localDW->is_winStates = power_window_control_IN_Stopped;

						/* Entry 'Stopped': '<S1>:3' */
						*rty_cmd = 0U;
						localDW->is_Stopped = power_window_c_IN_partiallyOpen;
					}
				}
			}

			else {
				/* During 'movingUpX': '<S1>:12' */
				if (localDW->is_movingUpX == power_window_controll_IN_mux_pp) {
					Dio_WriteChannel(DioConf_DioChannel_LED_1, IOHWAB_HIGH);
					Dio_WriteChannel(DioConf_DioChannel_LED_2, IOHWAB_HIGH);
					/* During 'mux_pp': '<S1>:15' */
					if (*rtu_obstacle == 1) {
						/* Transition: '<S1>:41' */
						/* Transition: '<S1>:40' */
						localDW->is_movingUpX = power_window_IN_NO_ACTIVE_CHILD;
						localDW->is_movingUp = power_window_IN_NO_ACTIVE_CHILD;
						localDW->is_winStates = power_window_cont_IN_retracting;

						/* Entry 'retracting': '<S1>:4' */
						*rty_cmd = 1U;
					}
				} else {
					Dio_WriteChannel(DioConf_DioChannel_LED_1, IOHWAB_LOW);
					Dio_WriteChannel(DioConf_DioChannel_LED_2, IOHWAB_HIGH);
					/* During 'mux_ppo': '<S1>:14' */
					if (*rtu_req != 4) {
						/* Transition: '<S1>:19' */
						localDW->is_movingUpX = power_window_controll_IN_mux_pp;
					}
				}
			}
			break;

		default:
			/* Chart: '<Root>/WinController' */
			/* During 'retracting': '<S1>:4' */
			Dio_WriteChannel(DioConf_DioChannel_LED_4, IOHWAB_HIGH);
			Dio_WriteChannel(DioConf_DioChannel_LED_3, IOHWAB_HIGH);
			Dio_WriteChannel(DioConf_DioChannel_LED_2, IOHWAB_LOW);
			Dio_WriteChannel(DioConf_DioChannel_LED_1, IOHWAB_LOW);
			if (*rtu_endStop) {
				/* Transition: '<S1>:118' */
				/* Transition: '<S1>:133' */
				localDW->is_winStates = power_window_control_IN_Stopped;

				/* Entry 'Stopped': '<S1>:3' */
				*rty_cmd = 0U;
				localDW->is_Stopped = power_window_contr_IN_fullyOpen;
			}
			break;
	}
}

/*
	switch (request)
	    {
	    case 0:
	    	Dio_WriteChannel(DioConf_DioChannel_LED_1, 0);
	    	Dio_WriteChannel(DioConf_DioChannel_LED_2, 0);
	    	Dio_WriteChannel(DioConf_DioChannel_LED_3, 0);
	    	Dio_WriteChannel(DioConf_DioChannel_LED_4, 0);
	    	break;
	    case 1:
	    	Dio_WriteChannel(DioConf_DioChannel_LED_1, 1);
	    	Dio_WriteChannel(DioConf_DioChannel_LED_2, 0);
	    	Dio_WriteChannel(DioConf_DioChannel_LED_3, 0);
	    	Dio_WriteChannel(DioConf_DioChannel_LED_4, 0);
	    	break;
	    case 2:
	    	Dio_WriteChannel(DioConf_DioChannel_LED_1, 0);
	    	Dio_WriteChannel(DioConf_DioChannel_LED_2, 1);
	    	Dio_WriteChannel(DioConf_DioChannel_LED_3, 0);
	    	Dio_WriteChannel(DioConf_DioChannel_LED_4, 0);
	    	break;
	    case 3:
	    	Dio_WriteChannel(DioConf_DioChannel_LED_1, 0);
	    	Dio_WriteChannel(DioConf_DioChannel_LED_2, 0);
	    	Dio_WriteChannel(DioConf_DioChannel_LED_3, 1);
	    	Dio_WriteChannel(DioConf_DioChannel_LED_4, 0);
	    	break;
	    case 4:
	    	Dio_WriteChannel(DioConf_DioChannel_LED_1, 0);
	    	Dio_WriteChannel(DioConf_DioChannel_LED_2, 0);
	    	Dio_WriteChannel(DioConf_DioChannel_LED_3, 0);
	    	Dio_WriteChannel(DioConf_DioChannel_LED_4, 1);
	    	break;
	    }
*/
