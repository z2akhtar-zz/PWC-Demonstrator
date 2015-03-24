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

#define NO_SOAD_TCPIP_EVENT_TYPE
#include "lwip/opt.h"
#include "lwip/tcp.h"
#include "lwip/udp.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/tcpip.h"
#include "lwip/netif.h"
#include "netif/etharp.h"
#include "lwip/dhcp.h"
#include "lwip/autoip.h"
#include "ethernetif.h"
#include "lwip_handler.h"
#include <stdio.h>
#include "netbios.h"

#include "Os.h"
#include "isr.h"
#include "irq.h"
#include "arc.h"
#include "Mcu.h"
#if defined(USE_SOAD)
#include "Soad.h"
#endif

#include "fec_5xxx.h"
#include "mpc55xx.h"

#define MAX_DHCP_TRIES        1

static struct netif netif;
static boolean tcpip_initialized = FALSE;


/* Eth Isr routine */
static void Eth_Isr(void)
{
	uint32_t res = 0;
	uint32 ievent;
	uint32 error = 0;

	/* Check events */
	ievent = FEC.EIR.R;

	if (ievent & (BABBLINGTRANSMIT | ETHBUS_ERROR | LATECOLLISION |
				  COLLISIONRETRYLIMIT | TRANSMITFIFOUNDERRUN)) {
		/* transmit errors */
		error = ievent;
	}
	if (ievent & HEARTBEAT_ERROR) {
		/* Heartbeat error */
		error = ievent;
	}
	if (ievent & GRACEFULSTOPACK) {
		/* Graceful stop complete */
		error = ievent;
	}

	while(fec_is_rx_data_available() &&  (res == 0))
	{
		  /* move received packet into a new pbuf */
		  struct pbuf *p = low_level_input(&netif);

          if(p!=NULL){
        	  tcpip_input(p, &netif);
          }
          else{
        	  res = 1;
          }
	}

	/* Clear events */
	FEC.EIR.R = ievent;

    FEC.RDAR.B.R_DES_ACTIVE = 1;
}

ISR(Eth_Isr_RXF){Eth_Isr();};
ISR(Eth_Isr_TXF){Eth_Isr();};
ISR(Eth_Isr_FEC){Eth_Isr();};

static void
tcpip_init_done(void *arg)
{
	tcpip_initialized = TRUE;
}

#if LWIP_NETIF_LINK_CALLBACK
void LwIP_LinkCallback(struct netif *netif) {
	// Status used instead
}
#endif


#if LWIP_NETIF_STATUS_CALLBACK
void LwIP_StatusCallback(struct netif *netif) {
	uint8 isLinkUp = netif_is_link_up(netif);
	struct ip_addr * newAddr = &netif->ip_addr;
	sint8* addrDataPtr = (sint8*) &(newAddr->addr);

#if defined(USE_SOAD)
	// IMPROVEMENT: determine IP interface index based on netif reference
	uint8 ipIfIndex = 0;
#endif

	SoAd_SockAddrType newSoadAddr = {
			.sa_len = sizeof(SoAd_SockAddrType),
			.sa_family = AF_INET,
	};
	newSoadAddr.sa_data[0] = addrDataPtr[0];
	newSoadAddr.sa_data[1] = addrDataPtr[1];
	newSoadAddr.sa_data[2] = addrDataPtr[2];
	newSoadAddr.sa_data[3] = addrDataPtr[3];

	if (isLinkUp) {
#if defined(USE_SOAD)
		SoAd_Cbk_LocalIpAssignmentChg(ipIfIndex, 1, newSoadAddr);
#endif
	} else {
#if defined(USE_SOAD)
		SoAd_Cbk_LocalIpAssignmentChg(ipIfIndex, 0, newSoadAddr);
#endif
	}
}
#endif

struct netif * LwIP_Init()
{
  uint8_t macaddress[6] = ETH_MAC_ADDR;
  struct ip_addr ipaddr;
  struct ip_addr netmask;
  struct ip_addr gw;

#ifdef BUILD_LOAD_MODULE
    uint32 *pBootAreaStart = (u32*)&boot_area_start;

    // Get boot programmed MAC address
    memcpy(macaddress,pBootAreaStart,6);

    *pBootAreaStart = 0x12345678;
    //IP4_ADDR((struct ip_addr *)(pBootAreaStart+1), 192, 168, 0, 8);
    //IP4_ADDR((struct ip_addr *)(pBootAreaStart+2), 255, 255, 255, 0);
    //IP4_ADDR((struct ip_addr *)(pBootAreaStart+3), 192, 168, 0, 1);
    *(pBootAreaStart+1) = 0; // use dhcp
#endif

  /* Create isr for ethernet interrupt */
    ISR_INSTALL_ISR2( "FecRXInt", Eth_Isr_RXF, FEC_RX, 3, 0 );
    ISR_INSTALL_ISR2( "FecTXInt", Eth_Isr_TXF, FEC_TX, 3, 0 );
    ISR_INSTALL_ISR2( "FecWorldInt", Eth_Isr_FEC, FEC_WORLD, 3, 0 );
  /*  EIR_TXF_INT = 194, EIR_RXF_INT = 195, EIR_FEC_INT = 196*/

  /* Configure ethernet */
   Ethernet_Configuration();

#if NO_SYS
#if (MEM_LIBC_MALLOC==0)  
  mem_init();
#endif
#if (MEMP_MEM_MALLOC==0)  
  memp_init();
#endif
#else
  pre_sys_init();
  Sleep(100);
  tcpip_init(tcpip_init_done, NULL);
  Sleep(100);
  uint32 lockcnt = 0;
  while(tcpip_initialized == FALSE){
  	 lockcnt++;
  	 Sleep(1);
  };
#endif

#if LWIP_DHCP
  ipaddr.addr = 0;
  netmask.addr = 0;
  gw.addr = 0;
#else
  GET_BOOT_IPADDR;
  GET_BOOT_NETMASK;
  GET_BOOT_GW;
#endif

  Set_MAC_Address(macaddress);

  /* Add network interface to the netif_list */
  netif_add(&netif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input);

  /*  Registers the default network interface.*/
  netif_set_default(&netif);
#if LWIP_NETIF_LINK_CALLBACK
  netif_set_link_callback(&netif, LwIP_LinkCallback);
#endif
#if LWIP_NETIF_STATUS_CALLBACK
  netif_set_status_callback(&netif, LwIP_StatusCallback);
#endif

#if LWIP_NETIF_HOSTNAME
#if defined(USE_SOAD)
  uint8* hostnameStr;
  uint8  hostnameLen;
  if (E_OK == SoAd_DoIp_Arc_GetHostname(&hostnameStr, &hostnameLen)) {
	  netif_set_hostname(&netif, (char*)hostnameStr);
  }
#else
  netif_set_hostname(&netif, "LWIP");
#endif
#endif


/* with LWIP_DHCP_AUTOIP_COOP enabled, you MUST NOT call autoip_start()
yourself! Only the dhcp code may call this or else the AutoIP state machine
might get out of sync.

With other words you must not use DHCP and AUTOIP at the same time unless you have LWIP_DHCP_AUTOIP_COOP enabled
and then dhcp_start will handle the autoip setup if it fails with dhcp

 * LWIP_DHCP_AUTOIP_COOP_TRIES: Set to the number of DHCP DISCOVER probes
 * that should be sent before falling back on AUTOIP. This can be set
 * as low as 1 to get an AutoIP address very quickly, but you should
 * be prepared to handle a changing IP address when DHCP overrides
 * AutoIP.
  */
#if (LWIP_DHCP == 1 && LWIP_AUTOIP == 1 && LWIP_DHCP_AUTOIP_COOP == 0)
	#error Must have LWIP_DHCP_AUTOIP_COOP = 1 if both dhcp and autoip are used at the same time.
#endif


#if LWIP_DHCP || LWIP_AUTOIP

#if LWIP_DHCP
  /* start dhcp search */
  dhcp_start(&netif);
#endif

#if LWIP_AUTOIP

#if LWIP_DHCP_AUTOIP_COOP
  /* dhcp_start will call autoip_start */
#else
  /* start autoip configuration */
  autoip_start(&netif);
#endif

#endif

#else
  /* Hardcoded addresses */
  //netif_set_addr(&netif, &ipaddr , &netmask, &gw);
  netif_set_up(&netif);
#endif


  fec_enable_reception();

  netbios_init();

  return &netif;
}
