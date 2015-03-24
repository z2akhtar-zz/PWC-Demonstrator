/*-------------------------------- Arctic Core ------------------------------
 * Copyright (C) 2013, ArcCore AB, Sweden, www.arccore.com.
 * Contact: <contact@arccore.com>
 *
 * You may ONLY use this file:
 * 1)if you have a valid commercial ArcCore license and then in accordance with
 * the terms contained in the written license agreement between you and ArcCore,
 * or alternatively
 * 2)if you follow the terms found in GNU General Public License version 2 as
 * published by the Free Software Foundation and appearing in the file
 * LICENSE.GPL included in the packaging of this file or here
 * <http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt>
 *-------------------------------- Arctic Core -----------------------------*/



#include "CanSM.h"
#include "CanIf.h"
#include "CanSM_Internal.h"
extern const CanSM_ConfigType* CanSM_ConfigPtr;

typedef union {
    CanIf_ControllerModeType ccMode;
#if (CANSM_CAN_TRCV_SUPPORT == STD_ON)
    CanTrcv_TrcvModeType trcvMode;
#endif
}CanSM_ActionMode;

typedef enum {
    A_CC_MODE = 0,
#if (CANSM_CAN_TRCV_SUPPORT == STD_ON)
    A_TRCV_MODE,
#endif
    A_NO_MODE
}CanSM_ActionType;

typedef struct {
    CanSM_ActionType FirstAction;
    CanSM_ActionMode FirstMode;
    CanSM_ActionType SecondAction;
    CanSM_ActionMode SecondMode;
}CanSM_TransitionType;

const CanSM_TransitionType TransitionConfig[] = {
        [T_CC_STOPPED_CC_SLEEP] = {
            .FirstAction = A_CC_MODE,
            .FirstMode = {.ccMode = CANIF_CS_STOPPED},
            .SecondAction = A_CC_MODE,
            .SecondMode = {.ccMode = CANIF_CS_SLEEP}
        },
#if (CANSM_CAN_TRCV_SUPPORT == STD_ON)
        [T_CC_SLEEP_TRCV_NORMAL] = {
            .FirstAction = A_CC_MODE,
            .FirstMode = {.ccMode = CANIF_CS_SLEEP},
            .SecondAction = A_TRCV_MODE,
            .SecondMode = {.ccMode = CANTRCV_TRCVMODE_NORMAL}
        },
        [T_TRCV_NORMAL_TRCV_STANDBY] = {
            .FirstAction = A_TRCV_MODE,
            .FirstMode = {.trcvMode = CANTRCV_TRCVMODE_NORMAL},
            .SecondAction = A_TRCV_MODE,
            .SecondMode = {.trcvMode = CANTRCV_TRCVMODE_STANDBY}
        },
        [T_TRCV_STANDBY_NONE] = {
            .FirstAction = A_TRCV_MODE,
            .FirstMode = {.trcvMode = CANTRCV_TRCVMODE_STANDBY},
            .SecondAction = A_NO_MODE,
            .SecondMode = {.ccMode = (CanIf_ControllerModeType)0} /* Don't care */
        },
        [T_TRCV_NORMAL_CC_STOPPED] = {
            .FirstAction = A_TRCV_MODE,
            .FirstMode = {.trcvMode = CANTRCV_TRCVMODE_NORMAL},
            .SecondAction = A_CC_MODE,
            .SecondMode = {.ccMode = CANIF_CS_STOPPED}
        },
#else
        [T_CC_SLEEP_NONE] = {
            .FirstAction = A_CC_MODE,
            .FirstMode = {.ccMode = CANIF_CS_SLEEP},
            .SecondAction = A_NO_MODE,
            .SecondMode = {.ccMode = (CanIf_ControllerModeType)0} /* Don't care */
        },
#endif
        [T_CC_STOPPED_CC_STARTED] = {
            .FirstAction = A_CC_MODE,
            .FirstMode = {.ccMode = CANIF_CS_STOPPED},
            .SecondAction = A_CC_MODE,
            .SecondMode = {.ccMode = CANIF_CS_STARTED}
        },
        [T_CC_STARTED_NONE] = {
            .FirstAction = A_CC_MODE,
            .FirstMode = {.ccMode = CANIF_CS_STARTED},
            .SecondAction = A_NO_MODE,
            .SecondMode = {.ccMode = (CanIf_ControllerModeType)0} /* Don't care */
        }
};

CanSM_Internal_TransitionReturnType CanSM_Internal_ExecuteTransition(CanSM_Internal_NetworkType *Network, NetworkHandleType NetworkHandle, CanSM_Internal_TransitionType transition) {
    CanSM_Internal_TransitionReturnType ret = T_WAIT_INDICATION;
    const CanSM_TransitionType *transitionPtr = &TransitionConfig[transition];
    if( !Network->requestAccepted ) {
        Network->RepeatCounter++;
        Std_ReturnType actionRet = E_NOT_OK;
        if( A_CC_MODE == transitionPtr->FirstAction ) {
            /* @req CANSM464 *//* PreNoCom CC_STOPPED */
            /* @req CANSM468 *//* PreNoCom CC_SLEEP */
            /* @req CANSM487 *//* PreFullCom CC_STOPPED */
            /* @req CANSM491 *//* PreFullCom CC_STARTED */
            /* @req CANSM509 *//* FullCom RESTART_CC */
            actionRet = CanSM_Internal_SetNetworkControllerMode(NetworkHandle, transitionPtr->FirstMode.ccMode);
#if (CANSM_CAN_TRCV_SUPPORT == STD_ON)
        } else if (A_TRCV_MODE == transitionPtr->FirstAction ) {
            actionRet = CanSM_Internal_SetNetworkTransceiverMode(NetworkHandle, transitionPtr->FirstMode.trcvMode);
#endif
        } else {
            CANSM_DET_REPORTERROR(CANSM_SERVICEID_MAINFUNCTION, CANSM_E_INVALID_ACTION);
        }
        if( E_OK == actionRet ) {
            /* @req CANSM465 *//* PreNoCom CC_STOPPED */
            /* @req CANSM469 *//* PreNoCom CC_SLEEP */
            /* @req CANSM488 *//* PreFullCom CC_STOPPED */
            /* @req CANSM492 *//* PreFullCom CC_STARTED */
            /* @req CANSM510 *//* FullCom RESTART_CC */
            Network->requestAccepted = TRUE;
            ret = T_REQ_ACCEPTED;
        } else {
            /* Request not accepted */
            ret = T_FAIL;
            if( Network->RepeatCounter > CanSM_ConfigPtr->CanSMModeRequestRepetitionMax ) {
                /* T_REPEAT_MAX */
                Network->RepeatCounter = 0;
                ret = T_REPEAT_MAX;
            }
        }
    } else {
        /* Check if mode has been indicated */
        Std_ReturnType indicationRet = E_NOT_OK;
        if( A_CC_MODE == transitionPtr->FirstAction ) {
            indicationRet = CanSM_Internal_GetNetworkControllerModeIndicated(NetworkHandle);
        }
#if (CANSM_CAN_TRCV_SUPPORT == STD_ON)
        else if( A_TRCV_MODE == transitionPtr->FirstAction ) {
            indicationRet = CanSM_Internal_GetTransceiverModeIndicated(NetworkHandle);
        }
#endif
        if(E_OK == indicationRet) {
            /* @req CANSM466 *//* PreNoCom CC_STOPPED */
            /* @req CANSM470 *//* PreNoCom CC_SLEEP */
            /* @req CANSM489 *//* PreFullCom CC_STOPPED */
            /* @req CANSM493 *//* PreFullCom CC_STARTED */
            /* @req CANSM511 *//* FullCom RESTART_CC */
            /* Mode indicated, new request */
            Network->RepeatCounter = 0;
            Network->requestAccepted = FALSE;
            Std_ReturnType actionRet = E_NOT_OK;
            if( A_CC_MODE == transitionPtr->SecondAction ) {
                actionRet = CanSM_Internal_SetNetworkControllerMode(NetworkHandle, transitionPtr->SecondMode.ccMode);
            }
#if (CANSM_CAN_TRCV_SUPPORT == STD_ON)
            else if( A_TRCV_MODE == transitionPtr->SecondAction ) {
                actionRet = CanSM_Internal_SetNetworkTransceiverMode(NetworkHandle, transitionPtr->SecondMode.trcvMode);
            }
#endif
            if( A_NO_MODE != transitionPtr->SecondAction ) {
                Network->RepeatCounter++;
                if( E_OK == actionRet ) {
                    /* Request accepted */
                    Network->requestAccepted = TRUE;
                    ret = T_DONE;
                } else {
                    /* Not accepted */
                    if( Network->RepeatCounter <= CanSM_ConfigPtr->CanSMModeRequestRepetitionMax ) {
                        /* proceed to next state state */
                        ret = T_DONE;
                    } else {
                        /* T_REPEAT_MAX */
                        ret = T_REPEAT_MAX;
                        Network->RepeatCounter = 0;
                    }
                }
            } else {
                /* No second action */
                ret = T_DONE;
            }
        } else if( Network->subStateTimer >= CanSM_ConfigPtr->CanSMModeRequestRepetitionTime ) {
            /* Timeout */
            /* @req CANSM467 *//* PreNoCom CC_STOPPED */
            /* @req CANSM471 *//* PreNoCom CC_SLEEP */
            /* @req CANSM490 *//* PreFullCom CC_STOPPED */
            /* @req CANSM494 *//* PreFullCom CC_STARTED */
            /* @req CANSM512 *//* FullCom RESTART_CC */
            Network->requestAccepted = FALSE;
            ret = T_FAIL;
            if( Network->RepeatCounter <= CanSM_ConfigPtr->CanSMModeRequestRepetitionMax ) {
                Network->RepeatCounter++;
                /* Try again */
                Std_ReturnType actionRet = E_NOT_OK;
                if( A_CC_MODE == transitionPtr->FirstAction ) {
                    actionRet = CanSM_Internal_SetNetworkControllerMode(NetworkHandle, transitionPtr->FirstMode.ccMode);
                }
#if (CANSM_CAN_TRCV_SUPPORT == STD_ON)
                else if( A_TRCV_MODE == transitionPtr->FirstAction ) {
                    actionRet = CanSM_Internal_SetNetworkTransceiverMode(NetworkHandle, transitionPtr->FirstMode.trcvMode);
                }
#endif
                if( E_OK == actionRet ) {
                    /* Request accepted */
                    Network->requestAccepted = TRUE;
                    ret = T_REQ_ACCEPTED;
                } else {
                    if( Network->RepeatCounter > CanSM_ConfigPtr->CanSMModeRequestRepetitionMax ) {
                        /* T_REPEAT_MAX */
                        /* @req CANSM480 *//* PreNoCom */
                        /* @req CANSM495 *//* PreFullCom */
                        /* @req CANSM523 *//* FullCom */
                        Network->RepeatCounter = 0;
                        ret = T_REPEAT_MAX;
                    } else {
                        /* Keep state */
                    }
                }
            } else {
                /* T_REPEAT_MAX */
                /* @req CANSM480 *//* PreNoCom */
                /* @req CANSM495 *//* PreFullCom */
                /* @req CANSM523 *//* FullCom */
                Network->RepeatCounter = 0;
                ret = T_REPEAT_MAX;
            }
        }
    }
    return ret;
}
