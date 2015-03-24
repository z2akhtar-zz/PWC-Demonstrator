#include "PWC.h"
#include "Rte_WinArbitratorType.h"

/* Block signals (auto storage) */
B_PWC_T PWC_B = {0};

extern void PWC_WinArbitrator(uint8_T rtu_u, uint8_T rtu_v, B_WinArbitrator_PWC_T *localB);

// Implementation code obtained from Runnable_Step in PWC.c
void WinArbitratorRunnable(void)
{
	uint8_T rtb_TmpSignalConversionAtdriver;
	uint8_T rtb_TmpSignalConversionAtpassen;

	 /* SignalConversion: '<Root>/TmpSignal ConversionAtdriverOutport1' incorporates:
		   *  Inport: '<Root>/driver'
		   */
    rtb_TmpSignalConversionAtdriver = Rte_IRead_WinArbitratorRunnable_req_d_request();

	/* SignalConversion: '<Root>/TmpSignal ConversionAtpassengerOutport1' incorporates:
	 *  Inport: '<Root>/passenger'
	 */
	rtb_TmpSignalConversionAtpassen = request_neutral;

	/* Truth Table: '<Root>/WinArbitrator' */
    PWC_WinArbitrator(rtb_TmpSignalConversionAtdriver, rtb_TmpSignalConversionAtpassen, &PWC_B.sf_WinArbitrator);

    Rte_IWrite_WinArbitratorRunnable_req_a_request(PWC_B.sf_WinArbitrator.y);
}

void PWC_WinArbitrator(uint8_T rtu_u, uint8_T rtu_v, B_WinArbitrator_PWC_T *localB)
{
  boolean_T b;
  boolean_T c;
  boolean_T d;
  boolean_T e;

  /* Truth Table Function 'WinArbitrator': '<S1>:1' */
  /*  Example condition 1 */
  if ((rtu_u == 0) && (rtu_v == 0)) {
    /* Condition '#1': '<S1>:1:11' */
    b = true;
  } else {
    /* Condition '#1': '<S1>:1:11' */
    b = false;
  }

  /*  Example condition 2 */
  if ((rtu_u == 0) && (rtu_v == 1)) {
    /* Condition '#2': '<S1>:1:15' */
    c = true;
  } else {
    /* Condition '#2': '<S1>:1:15' */
    c = false;
  }

  if ((rtu_u == 1) && (rtu_v == 0)) {
    /* Condition '#3': '<S1>:1:18' */
    d = true;
  } else {
    /* Condition '#3': '<S1>:1:18' */
    d = false;
  }

  if ((rtu_u == 1) && (rtu_v == 1)) {
    /* Condition '#4': '<S1>:1:21' */
    e = true;
  } else {
    /* Condition '#4': '<S1>:1:21' */
    e = false;
  }

  if (b && (!c) && d && e) {
    /* Condition '#1': '<S1>:1:11' */
    /* Decision 'D1': '<S1>:1:23' */
    /* Condition '#3': '<S1>:1:18' */
    /* Condition '#4': '<S1>:1:21' */
    /*  Example action 1 called from D1 & D2 column in condition table */
    /* Action '1': '<S1>:1:35' */
    localB->y = rtu_u;
  } else {
    /* Decision 'D2': '<S1>:1:23' */
    if ((!b) && c && (!d) && (!e)) {
      /* Decision 'D2': '<S1>:1:25' */
      /* Condition '#2': '<S1>:1:15' */
      /* Decision 'D2': '<S1>:1:25' */
      /*  Example action 2 called from D3 column in condition table */
      /* Action '2': '<S1>:1:41' */
      localB->y = rtu_v;
    } else {
      /* Decision 'D3': '<S1>:1:25' */
      /* Decision 'D3': '<S1>:1:27' */
      /*  Default */
      /* Action '3': '<S1>:1:46' */
      localB->y = rtu_u;
    }
  }
}

