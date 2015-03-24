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


/** @reqSettings DEFAULT_SPECIFICATION_REVISION=3.1.5 */


/** @req NVM077 */


#ifndef NVM_H_
#define NVM_H_

#include "Modules.h"

#define NVM_MODULE_ID			MODULE_ID_NVM
#define NVM_VENDOR_ID			VENDOR_ID_ARCCORE

#define NVM_SW_MAJOR_VERSION	1u
#define NVM_SW_MINOR_VERSION	2u
#define NVM_SW_PATCH_VERSION	0u
#define NVM_AR_MAJOR_VERSION	3u
#define NVM_AR_MINOR_VERSION	1u
#define NVM_AR_PATCH_VERSION	5u

#include "NvM_Cfg.h"
/** @req NVM028 */ // NvmCommon configuration block is implemented in the NvM_Cfg.h file
/** @req NVM491 *//** @req NVM492 *//** @req NVM493 *//** @req NVM494 *//** @req NVM495 */
/** @req NVM496 *//** @req NVM497 *//** @req NVM498 *//** @req NVM499 */
/** @req NVM501 *//** @req NVM502 *//** @req NVM503 *//** @req NVM504 *//** @req NVM505 */


#if (NVM_DEV_ERROR_DETECT == STD_ON)
// Error codes reported by this module defined by AUTOSAR
#define NVM_E_PARAM_BLOCK_ID				0x0Au
#define NVM_E_PARAM_BLOCK_TYPE				0x0Bu
#define NVM_E_PARAM_BLOCK_DATA_IDX			0x0Cu
#define NVM_E_PARAM_ADDRESS					0x0Du
#define NVM_E_PARAM_DATA					0x0Eu
#define NVM_E_NOT_INITIALIZED				0x14u
#define NVM_E_BLOCK_PENDING					0x15u
#define NVM_E_LIST_OVERFLOW					0x16u
#define NVM_E_NV_WRITE_PROTECTED			0x17u
#define NVM_E_BLOCK_CONFIG					0x18u


// Other error codes reported by this module
#define NVM_PARAM_OUT_OF_RANGE				0x40u
#define NVM_UNEXPECTED_STATE				0x41u
#define NVM_E_WRONG_CONFIG					0xfdu
#define NVM_E_UNEXPECTED_EXECUTION			0xfeu
#define NVM_E_NOT_IMPLEMENTED_YET			0xffu

#endif
// Service ID in this module
#define NVM_INIT_ID								0x00u
#define NVM_SET_DATA_INDEX_ID					0x01u
#define NVM_GET_DATA_INDEX_ID					0x02u
#define NVM_SET_BLOCK_PROTECTION_ID				0x03u
#define NVM_GET_ERROR_STATUS_ID					0x04u
#define NVM_SET_RAM_BLOCK_STATUS_ID				0x05u
#define NVM_READ_BLOCK_ID						0x06u
#define NVM_WRITE_BLOCK_ID						0x07u
#define NVM_RESTORE_BLOCK_DEFAULTS_ID			0x08u
#define NVM_ERASE_NV_BLOCK_ID					0x09u
#define NVM_CANCEL_WRITE_ALL_ID					0x0au
#define NVM_INVALIDATENV_BLOCK_ID 				0x0bu
#define NVM_READ_ALL_ID							0x0cu
#define NVM_WRITE_ALL_ID						0x0du
#define NVM_MAIN_FUNCTION_ID					0x0eu
#define NVM_GET_VERSION_INFO_ID					0x0fu

#define NVM_LOC_READ_BLOCK_ID					0x40u
#define NVM_LOC_WRITE_BLOCK_ID					0x41u
#define NVM_GLOBAL_ID							0xffu



#if ( NVM_VERSION_INFO_API == STD_ON )	/** @req NVM452 */
#define NvM_GetVersionInfo(_vi) STD_GET_VERSION_INFO(_vi, NVM)
#endif /* NVM_VERSION_INFO_API */

void NvM_MainFunction(void);	/** @req NVM464 */

void NvM_Init( void );	/** @req NVM447 */
void NvM_ReadAll( void );	/** @req NVM460 */
void NvM_WriteAll( void );	/** @req NVM461 */
void NvM_CancelWriteAll( void );	/** @req NVM458 */
Std_ReturnType NvM_GetErrorStatus( NvM_BlockIdType blockId, NvM_RequestResultType *requestResultPtr );	/** @req NVM451 */
void NvM_SetBlockLockStatus( NvM_BlockIdType blockId, boolean blockLocked );

#if (NVM_SET_RAM_BLOCK_STATUS_API == STD_ON)
Std_ReturnType NvM_SetRamBlockStatus( NvM_BlockIdType blockId, boolean blockChanged );	/** @req NVM453 */
#endif

#if (NVM_API_CONFIG_CLASS > NVM_API_CONFIG_CLASS_1)
Std_ReturnType NvM_SetDataIndex( NvM_BlockIdType blockId, uint8 dataIndex );	/** @req NVM448 */
Std_ReturnType NvM_GetDataIndex( NvM_BlockIdType blockId, uint8 *dataIndexPtr );	/** @req NVM449 */
Std_ReturnType NvM_ReadBlock( NvM_BlockIdType blockId, void *dstPtr );	/** @req NVM454 */
Std_ReturnType NvM_WriteBlock( NvM_BlockIdType blockId, void *srcPtr );	/** @req NVM455 */
Std_ReturnType NvM_RestoreBlockDefaults( NvM_BlockIdType blockId, void *dstPtr );	/** @req NVM456 */
#endif

#if (NVM_API_CONFIG_CLASS > NVM_API_CONFIG_CLASS_2)
Std_ReturnType NvM_SetBlockProtection( NvM_BlockIdType blockId, boolean protectionEnabled );	/** @req NVM450 */
Std_ReturnType NvM_EraseNvBlock( NvM_BlockIdType blockId );	/** @req NVM457 */
Std_ReturnType NvM_InvalidateNvBlock( NvM_BlockIdType blockId );	/** @req NVM459 */
#endif




#endif /*NVM_H_*/
