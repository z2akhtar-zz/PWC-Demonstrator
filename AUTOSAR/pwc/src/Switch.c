#include "PWC.h"
#include "Rte_SwitchType.h"

void SwitchRunnable(void)
{
	requestType myRequest = 0;
	static JOY_State_TypeDef JoyState = JOY_NONE;

	/* Get the Joytick State */
	JoyState = IOE_JoyStickGetState();

	switch (JoyState)
	{
	case JOY_NONE:
		myRequest = request_neutral;
		break;
	case JOY_UP:
		myRequest = request_basic_up;
		break;
	case JOY_DOWN:
		myRequest = request_basic_down;
		break;
	case JOY_LEFT:
		myRequest = request_express_down;
		break;
	case JOY_RIGHT:
		myRequest = request_express_up;
		break;
	case JOY_CENTER:
		break;
	default:
		break;
	}

	Rte_IWrite_SwitchRunnable_req_request(myRequest);
}
