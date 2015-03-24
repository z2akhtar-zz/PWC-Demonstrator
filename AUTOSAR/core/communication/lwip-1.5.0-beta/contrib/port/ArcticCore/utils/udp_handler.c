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



/* Includes ------------------------------------------------------------------*/
#include "Os.h"
#include "Mcu.h"

#include "lwip_handler.h"
#include "lwip/sockets.h"
#include "string.h"
#include "udp_handler.h"

/* Private macro -------------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
void UdpTickHandler(void);

#define MAX_NUM_OF_SOCKETS 10
static int sockList[MAX_NUM_OF_SOCKETS];
static UdpCbkFunc cbkFuncList[MAX_NUM_OF_SOCKETS] = {NULL};

/*
 * Function: KillUdpSocket
 * Description: Kill a socket to an UDP port
 * Param: Handle to the socket
 */
void KillUdpSocket(int socket)
{
	imask_t val;
	Irq_Save(val);
	for(int i=0;i<MAX_NUM_OF_SOCKETS;i++){
		if(sockList[i] == socket){
			sockList[i] = (-1);
		}
	}
	Irq_Restore(val);
	lwip_close(socket);
}

/*
 * Function: CreateUdpSocket
 * Description: Create a socket to an UDP port
 * Return: Handle to the socket
 */
int CreateUdpSocket(uint16_t port, UdpCbkFunc cbkFunc)
{
	int udpSocket;
	struct sockaddr_in sLocalAddr;
	imask_t val;

	  /* Init socket */
	udpSocket = lwip_socket(AF_INET, SOCK_DGRAM, 0);

	Irq_Save(val);
	for(int i=0;i<MAX_NUM_OF_SOCKETS;i++){
		if(sockList[i] == (-1)){
			sockList[i] = udpSocket;
			cbkFuncList[i] = cbkFunc;
			break;
		}
	}
	Irq_Restore(val);

	memset((char *)&sLocalAddr, 0, sizeof(sLocalAddr));

	/*Source*/
	sLocalAddr.sin_family = AF_INET;
	sLocalAddr.sin_len = sizeof(sLocalAddr);
	sLocalAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	sLocalAddr.sin_port = htons(port);

	if(lwip_bind(udpSocket, (struct sockaddr *)&sLocalAddr, sizeof(sLocalAddr)) < 0)
	{
		KillUdpSocket(udpSocket);
	}

	return udpSocket;
}

void UdpSendData(int socket, const void *data, uint16_t size, uint16_t port)
{
	struct sockaddr_in sDestAddr;

	memset((char *)&sDestAddr, 0, sizeof(sDestAddr));

	/*Destination*/
	sDestAddr.sin_family = AF_INET;
	sDestAddr.sin_len = sizeof(sDestAddr);
	sDestAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
	sDestAddr.sin_port = htons(port);

	// Send some data to show we are alive
	lwip_sendto(socket, data, size, 0, (struct sockaddr *)&sDestAddr, sizeof(sDestAddr));
}

static uint8_t buffer[1500];
void UdpReceiveData()
{
	int nbytes;

	for(int i=0;i<MAX_NUM_OF_SOCKETS;i++){
		if(sockList[i] != (-1)){
			nbytes=lwip_recv(sockList[i], buffer, sizeof(buffer),MSG_DONTWAIT);
			if (nbytes>0){
				if(cbkFuncList[i] != NULL)
				{
					cbkFuncList[i](buffer, nbytes);
				}
			}
		}
	}
}

void InitUdpHandler()
{
    for(int i=0;i<MAX_NUM_OF_SOCKETS;i++){
        sockList[i] = (-1);
    }
}

/*******************************************************************************
* Function Name  : UdpTickHandler.
* Description    : This function is the Udp task implementation
*******************************************************************************/
void UdpTask(void)
{
    /* Receive udp data for all sockets connected */
    UdpReceiveData();
	TerminateTask();
}





