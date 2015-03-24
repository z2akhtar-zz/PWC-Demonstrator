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
#include "tcp_handler.h"

/* Private macro -------------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
void TcpTickHandler(void);

static int sock = (-1);
static TcpCbkFunc tcpCbkFunc = NULL;
static int clientfd = (-1);

/*
 * Function: KilltcpSocket
 * Description: Kill a socket to an tcp port
 * Param: Handle to the socket
 */
void KillTcpSocket(int socket)
{
	imask_t val;
	Irq_Save(val);
	lwip_close(socket);
	sock = (-1);
	Irq_Restore(val);
}

/*
 * Function: CreatetcpSocket
 * Description: Create a socket to an tcp port
 * Return: Handle to the socket
 */
int CreateTcpSocket(uint16_t port, TcpCbkFunc cbkFunc)
{
	int tcpSocket;
	struct sockaddr_in sLocalAddr;
	imask_t val;

	  /* Init socket */
	tcpSocket = lwip_socket(AF_INET, SOCK_STREAM, 0);

	Irq_Save(val);
	sock = tcpSocket;
	tcpCbkFunc = cbkFunc;
	Irq_Restore(val);

	memset((char *)&sLocalAddr, 0, sizeof(sLocalAddr));

	/*Source*/
	sLocalAddr.sin_family = AF_INET;
	sLocalAddr.sin_len = sizeof(sLocalAddr);
	sLocalAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	sLocalAddr.sin_port = htons(port);

	if(lwip_bind(tcpSocket, (struct sockaddr *)&sLocalAddr, sizeof(sLocalAddr)) < 0)
	{
		KillTcpSocket(tcpSocket);
	}

	int timeout = 10000; /* 10000 msecs */
	lwip_setsockopt(tcpSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));


    if  ( lwip_listen(tcpSocket, 20) != 0 ){
    	KillTcpSocket(tcpSocket);
    }

    ActivateTask(TASK_ID_TcpTask);

	return tcpSocket;
}

void TcpSendData(const void *data, uint16_t size)
{
	if(clientfd != (-1))
	{
		lwip_send(clientfd, data, size, 0);
	}
}

static uint8_t buffer[1500];
/*******************************************************************************
* Function Name  : tcpTickHandler.
* Description    : This function is the tcp task implementation
*******************************************************************************/
void TcpTask( void )
{
	struct sockaddr_in client_addr;
	int addrlen=sizeof(client_addr);
    int nbytes;

    while(sock != (-1))
    {
    	/* accept will halt and wait for connection */
        clientfd = lwip_accept(sock, (struct sockaddr*)&client_addr, (socklen_t *)&addrlen);

        if( clientfd != (-1))
        {
			int timeout = 1000; /* 1000 msecs */
			lwip_setsockopt(clientfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

			do{
				/* receive will halt and wait for data */
				nbytes=lwip_recv(clientfd, buffer, sizeof(buffer),0);
				if (nbytes>0){
					if(tcpCbkFunc != NULL)
					{
						tcpCbkFunc(buffer, nbytes);
					}
				}
			}while(nbytes > 0);
			lwip_close(clientfd);
			clientfd = (-1);
        }
    }

    TerminateTask();
}
