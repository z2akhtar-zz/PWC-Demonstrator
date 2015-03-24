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

#ifndef SOAD_H
#define SOAD_H

#include "Modules.h"

#define SOAD_VENDOR_ID		   VENDOR_ID_ARCCORE
#define SOAD_AR_RELEASE_MAJOR_VERSION      4u
#define SOAD_AR_RELEASE_MINOR_VERSION      0u
#define SOAD_AR_RELEASE_REVISION_VERSION   3u

#define SOAD_MODULE_ID         MODULE_ID_SOAD
#define SOAD_AR_MAJOR_VERSION  SOAD_AR_RELEASE_MAJOR_VERSION
#define SOAD_AR_MINOR_VERSION  SOAD_AR_RELEASE_MINOR_VERSION
#define SOAD_AR_PATCH_VERSION  SOAD_AR_RELEASE_REVISION_VERSION

#define SOAD_SW_MAJOR_VERSION   1u
#define SOAD_SW_MINOR_VERSION   0u
#define SOAD_SW_PATCH_VERSION   0u

#define DOIP_VENDOR_ID          1u
#define DOIP_MODULE_ID          MODULE_ID_DOIP
#define DOIP_AR_MAJOR_VERSION   4u
#define DOIP_AR_MINOR_VERSION   0u
#define DOIP_AR_PATCH_VERSION   2u

#define DOIP_SW_MAJOR_VERSION   1u
#define DOIP_SW_MINOR_VERSION   0u
#define DOIP_SW_PATCH_VERSION   0u

#define TCPIP_VENDOR_ID          1u
#define TCPIP_MODULE_ID          MODULE_ID_TCPIP
#define TCPIP_AR_MAJOR_VERSION   4u
#define TCPIP_AR_MINOR_VERSION   0u
#define TCPIP_AR_PATCH_VERSION   2u

#define TCPIP_SW_MAJOR_VERSION   1u
#define TCPIP_SW_MINOR_VERSION   0u
#define TCPIP_SW_PATCH_VERSION   0u

#include "SoAd_Types.h"
#include "SoAd_Cbk.h"
#include "SoAd_Cfg.h"
#include "SoAd_Callouts.h"
#include "SoAd_ConfigTypes.h"

//#include "Bsd_Types.h"
#include "ComStack_Types.h"

#if defined(USE_PDUR)
#include "PduR.h"
#endif

#if defined(USE_COM)
#include "Com.h"
#endif

#if (SOAD_DEV_ERROR_DETECT == STD_ON)
// Error codes produced by this module defined by Autosar
#define SOAD_E_NOTINIT				0x01u
#define SOAD_E_NOENT				0x02u
#define SOAD_WRONG_PARAM_VAL		0x03u
#define SOAD_E_NULL_PTR				0x06u
#define SOAD_E_BADF					0x09u
#define SOAD_E_DEADLK				0x0Bu
#define SOAD_E_NOMEM				0x0Cu
#define SOAD_E_ACCES				0x0Du
#define SOAD_E_NOTDIR				0x14u
#define SOAD_E_ISDIR				0x15u
#define SOAD_E_INVAL				0x16u
#define SOAD_E_NFILE				0x17u
#define SOAD_E_MFILE				0x18u
#define SOAD_E_ROFS					0x1Eu
#define SOAD_E_DOM					0x21u
#define SOAD_E_WOULDBLOCK			0x22u
#define SOAD_E_INPROGRESS			0x24u
#define SOAD_E_ALREADY				0x25u
#define SOAD_E_NOTSOCK				0x26u
#define SOAD_E_DESTADDRREQ			0x27u
#define SOAD_E_MSGSIZE				0x28u
#define SOAD_E_PROTOTYPE			0x29u
#define SOAD_E_NOPROTOOPT			0x2Au
#define SOAD_E_PROTONOSUPPORT		0x2Bu
#define SOAD_E_OPNOTSUPP			0x2Du
#define SOAD_E_NOTSUP				0x2Eu
#define SOAD_E_AFNOSUPPORT			0x2Fu
#define SOAD_E_ADDRINUSE			0x30u
#define SOAD_E_ADDRNOTAVAIL			0x31u
#define SOAD_E_NOBUFS				0x37u
#define SOAD_E_ISCONN				0x38u
#define SOAD_E_LOOP					0x3Du
#define SOAD_E_NAMETOOLONG			0x3Fu
#define SOAD_E_NOLCK				0x4Du
#define SOAD_E_OVERFLOW				0x54u
#define SOAD_E_TCPIPUNKNOWN			0x5Au
#define SOAD_E_PDU2LONG				0x5Bu
#define SOAD_E_NOCONNECTOR			0x5Cu
#define SOAD_E_INVALID_TXPDUID		0x5Du
#define SOAD_E_PARAM_POINTER		0x5Eu

// Other error codes reported by this module
#define SOAD_E_UL_RXBUFF			0xfau
#define SOAD_E_CONFIG_INVALID		0xfbu
#define SOAD_E_UNEXPECTED_EXECUTION	0xfcu
#define SOAD_E_SHALL_NOT_HAPPEN		0xfdu
#define SOAD_E_NOT_SUPPORTED		0xfeu
#define SOAD_E_NOT_IMPLEMENTED_YET	0xffu

// Service IDs in this module defined by Autosar
#define SOAD_INIT_ID                		0x01u
#define SOAD_SOCKET_RESET_ID        		0x07u
#define SOAD_IF_TRANSMIT_ID          		0x08u
#define SOAD_SHUTDOWN_ID            		0x09u
#define SOAD_GET_VERSION_INFO_ID			0x0bu
#define SOAD_TP_TRANSMIT_ID          		0x0fu
#define SOAD_MAIN_FUNCTION_ID       		0x10u

#define DOIP_GET_VERSION_INFO_ID			0x60u

#define TCPIP_INIT_ID               		0x80u
#define TCPIP_SHUTDOWN_ID           		0x81u
#define TCPIP_GET_VERION_INFO_ID			0x8au
#define TCPIP_MAIN_FUNCTION_CYCLIC_ID 		0x8bu
#define TCPIP_SET_DHCP_HOST_NAME_OPTION_ID	0x89u

// Other service IDs reported by this module
#define SOAD_SOCKET_TCP_READ_ID				0xa0u
#define SOAD_SOCKET_UDP_READ_ID				0xa1u
#define SOAD_SCAN_SOCKETS_ID				0xa2u
#define SOAD_SOCKET_CLOSE_ID				0xa3u

#define SOAD_DOIP_HANDLE_DIAG_MSG_ID		0xb0u
#define SOAD_DOIP_CREATE_AND_SEND_NACK_ID	0xb1u
#define SOAD_DOIP_CREATE_AND_SEND_D_ACK_ID	0xb2u
#define SOAD_DOIP_CREATE_AND_SEND_D_NACK_ID	0xb3u
#define SOAD_DOIP_HANDLE_VEHICLE_ID_REQ_ID	0xb4u
#define SOAD_DOIP_ROUTING_ACTIVATION_REQ_ID	0xb5u
#define SOAD_DOIP_HANDLE_TP_TRANSMIT_ID		0xb6u
#define SOAD_DOIP_HANDLE_TCP_RX_ID			0xb7u
#define SOAD_DOIP_HANDLE_UDP_RX_ID			0xb8u
#define SOAD_DOIP_ENTITY_STATUS_REQ_ID			0xb9u
#define SOAD_DOIP_CREATE_AND_SEND_ALIVE_CHECK_ID 0xbau
#define DOIP_HANDLE_ALIVECHECK_RESP 0xbbu
#define DOIP_HANDLE_ALIVECHECK_TIMEOUT 0xbcu
#endif




void SoAd_Init(void);	/** @req SOAD093 */
Std_ReturnType SoAd_Shutdown(void);	/** @req SOAD092 */
void SoAd_MainFunction(void);	/** @req SOAD121 */
void SoAd_SocketReset(void);	/** @req SOAD127 */
Std_ReturnType SoAdIf_Transmit(PduIdType SoAdSrcPduId, const PduInfoType* SoAdSrcPduInfoPtr);	/** @req SOAD091 */
Std_ReturnType SoAdTp_Transmit(PduIdType SoAdSrcPduId, const PduInfoType* SoAdSrcPduInfoPtr);	/** @req SOAD105 */

void TcpIp_Init(void);	/** @req SOAD193 */
void TcpIp_Shutdown(void);	/** @req SOAD194 */
void TcpIp_MainFunctionCyclic(void);	/** @req SOAD143 */
Std_ReturnType TcpIp_SetDhcpHostNameOption(uint8* HostNameOption, uint8 HostNameLen);	/** @req SOAD196*/

void DoIp_Init();


#if ( SOAD_VERSION_INFO_API == STD_ON )
#define SoAd_GetVersionInfo(_vi) STD_GET_VERSION_INFO(_vi,SOAD)	/** @req SOAD096 */
#endif

#if ( DOIP_VERSION_INFO_API == STD_ON )
#define DoIp_GetVersionInfo(_vi) STD_GET_VERSION_INFO(_vi,DOIP)	/** @req SOAD095 */
#endif

#if ( TCPIP_VERSION_INFO_API == STD_ON )
#define TcpIp_GetVersionInfo(_vi) STD_GET_VERSION_INFO(_vi,TCPIP)	/** @req SOAD094 */
#endif

#endif
