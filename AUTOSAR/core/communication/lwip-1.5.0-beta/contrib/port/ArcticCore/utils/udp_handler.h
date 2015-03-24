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

#ifndef UDP_HANDLER_H_
#define UDP_HANDLER_H_

typedef void (*UdpCbkFunc)(const void*, uint16_t len);

int CreateUdpSocket(uint16_t port, UdpCbkFunc cbkFunc);
void UdpSendData(int socket, const void *data, uint16_t size, uint16_t port);
void InitUdpHandler();
void KillUdpSocket(int socket);
void UdpTask(void);

#endif /* UDP_HANDLER_H_ */
