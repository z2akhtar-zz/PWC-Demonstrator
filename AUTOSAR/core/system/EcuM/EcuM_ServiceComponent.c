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

#include "EcuM.h"
#include "EcuM_Generated_Types.h"
#include "EcuM_Internals.h"

Std_ReturnType EcuM_ShutdownTarget_GetLastShutdownTarget(EcuM_StateType* target, uint8* mode) {
	/*lint -estring(920,pointer)  cast to void */
	/*lint -save -e522 -e550 -e602 */
	(void)target;
    (void)mode;
    /*lint -restore */
	/*lint +estring(920,pointer)   */
	return E_NOT_OK;
}

Std_ReturnType EcuM_ShutdownTarget_GetShutdownTarget(EcuM_StateType* target, uint8* mode) {
	return EcuM_GetShutdownTarget(target, (uint8*)mode);
}

Std_ReturnType EcuM_ShutdownTarget_SelectShutdownTarget(EcuM_StateType target, uint8 mode) {
	return EcuM_SelectShutdownTarget(target, (uint8)mode);
}

Std_ReturnType EcuM_StateRequest_RequestRUN(EcuM_UserType user) {
	return EcuM_RequestRUN(user);
}

Std_ReturnType EcuM_StateRequest_ReleaseRUN(EcuM_UserType user) {
	return EcuM_ReleaseRUN(user);
}

Std_ReturnType EcuM_StateRequest_RequestPOSTRUN(EcuM_UserType user) {
	return EcuM_RequestPOST_RUN(user);
}

Std_ReturnType EcuM_StateRequest_ReleasePOSTRUN(EcuM_UserType user) {
	return EcuM_ReleasePOST_RUN(user);
}

Std_ReturnType EcuM_BootTarget_GetBootTarget(EcuM_BootTargetType* target) {
	return EcuM_GetBootTarget(target);
}

Std_ReturnType EcuM_BootTarget_SelectBootTarget(EcuM_BootTargetType target) {
	return EcuM_SelectBootTarget(target);
}
