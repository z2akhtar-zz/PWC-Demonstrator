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
#ifndef SD_H_
#define SD_H_

#include "SD_Types.h"
#include "SD_Cfg.h"

void Sd_Init( const Sd_ConfigType* ConfigPtr );
void Sd_GetVersionInfo( Std_VersionInfoType* versioninfo );
Std_ReturnType Sd_ServerServiceSetState( uint16 SdServerServiceHandleId, Sd_ServerServiceSetStateType ServerServiceState);
Std_ReturnType Sd_ClientServiceSetState( uint16 ClientServiceInstanceID, Sd_ClientServiceSetStateType ClientServiceState );
Std_ReturnType Sd_ConsumedEventGroupSetState( uint16 SdConsumedEventGroupHandleId, Sd_ConsumedEventGroupSetStateType ConsumedEventGroupState );
void Sd_LocalIpAddrAssignmentChg( SoAd_SoConIdType SoConId, TcpIp_IpAddrStateType State );

void Sd_RxIndication( PduIdType RxPduId, PduInfoType* PduInfoPtr);

void Sd_MainFunction( void );

#endif /* SD_H_ */
