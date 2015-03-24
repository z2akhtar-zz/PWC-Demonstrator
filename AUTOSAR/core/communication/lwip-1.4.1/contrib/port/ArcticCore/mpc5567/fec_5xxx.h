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


#ifndef FEC_5XXX_H_
#define FEC_5XXX_H_

#include "Std_Types.h"

/* Interrupt Event register */
#define	HEARTBEAT_ERROR    0x80000000
#define	BABBLINGRECEIVER   0x40000000
#define	BABBLINGTRANSMIT   0x20000000
#define	GRACEFULSTOPACK    0x10000000
#define	TRANSMITFRAMEINT   0x08000000
#define	TRANSMITBUFINT     0x04000000
#define	RECEIVEFRAMEINT    0x02000000
#define	RECEIVEBUFINT	   0x01000000
#define	MIIIINT 	   	   0x00800000
#define	ETHBUS_ERROR  	   0x00400000
#define	LATECOLLISION  	   0x00200000
#define	COLLISIONRETRYLIMIT 0x00100000
#define	TRANSMITFIFOUNDERRUN 0x00080000

int fec_mii_read(uint8 phyAddr, uint8 regAddr, uint16 * retVal);
int fec_mii_write(uint8 phyAddr, uint8 regAddr, uint32 data);
int fec_init(uint8 *macAddr);
int fec_init_phy(uint8 phyAddr);
int fec_send(uint8 *buf, uint16 len);
int fec_recv();
void fec_set_macaddr(uint8 *macAddr);
uint8 * fec_get_buffer_to_send();
boolean fec_is_rx_data_available();
void fec_enable_reception();

#endif	/* MPC5XXX_FEC_H_ */
