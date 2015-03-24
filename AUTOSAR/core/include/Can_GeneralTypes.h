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

/** @reqSettings DEFAULT_SPECIFICATION_REVISION=4.1.2 */
/** @tagSettings DEFAULT_ARCHITECTURE=PPC|RH850F1H|MPC5607B|MPC5645S */

#ifndef CAN_GENERALTYPES_H_
#define CAN_GENERALTYPES_H_

/** @req 4.1.2/SWS_Can_00436 Shared types between Can, CanIf, and CanTrcv */
/** @req 4.1.2/SWS_Can_00439 */
/** @req 4.1.2/SWS_Can_00487 */

/** @req 4.0.3/CAN429 *//** @req 4.1.2/SWS_Can_00429 */
typedef uint8 Can_HwHandleType;

/** @req 4.0.3/CAN039 *//** @req 4.1.2/SWS_Can_00039 */
typedef enum {
    CAN_OK,
    CAN_NOT_OK,
    CAN_BUSY
} Can_ReturnType;

/** @req 4.0.3/CAN417 *//** @req 4.1.2/SWS_Can_00417 */
typedef enum {
    CAN_T_START,
    CAN_T_STOP,
    CAN_T_SLEEP,
    CAN_T_WAKEUP
} Can_StateTransitionType;

/** @req 4.0.3/CAN416 *//** @req 4.1.2/SWS_Can_00416 */
// uint16: if only Standard IDs are used
// uint32: if also Extended IDs are used
typedef uint32 Can_IdType;

/** @req 4.0.3/CAN415 *//** @req 4.1.2/SWS_Can_00415 */
typedef struct Can_PduType_s {
    // private data for CanIf,just save and use for callback
    PduIdType   swPduHandle;
    // Length, max 8 bytes
    uint8       length;
    // the CAN ID, 29 or 11-bit
    Can_IdType  id;
    // data ptr
    uint8       *sdu;
} Can_PduType;


#endif /* CAN_GENERALTYPES_H_ */
