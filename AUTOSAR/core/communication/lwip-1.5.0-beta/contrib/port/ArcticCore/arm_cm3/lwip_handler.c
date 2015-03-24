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


#include "lwip/opt.h"
#include "lwip/tcp.h"
#include "lwip/udp.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/tcpip.h"
#include "netif/etharp.h"
#include "lwip/dhcp.h"
#include "ethernetif.h"
#include "lwip_handler.h"
#include <stdio.h>
#include "netbios.h"

#include "stm32_eth.h"

#include "Os.h"
#include "irq.h"
#include "arc.h"
#include "sleep.h"
#include "Mcu.h"

#define MAX_DHCP_TRIES        1

static struct netif netif;

err_t ethernetif_input(struct netif *netif, struct pbuf *p);

/* Eth Isr routine */
static void Eth_Isr(void)
{
	uint32_t res = 0;

	while((ETH_GetRxPktSize() != 0) && (res == 0))
	{
		  /* move received packet into a new pbuf */
		  struct pbuf *p = low_level_input(&netif);

          if(p!=NULL){
        	  tcpip_input(p, &netif);
          }else{
        	  res = 1;
          }
	}

	/* Clear the Eth DMA Rx IT pending bits */
	ETH_DMAClearITPendingBit(ETH_DMA_IT_R);
	ETH_DMAClearITPendingBit(ETH_DMA_IT_NIS);
	ETH_DMAClearITPendingBit(ETH_DMA_IT_RO);
	ETH_DMAClearITPendingBit(ETH_DMA_IT_RBU);
}

static boolean tcpip_initialized = FALSE;

static void
tcpip_init_done(void *arg)
{
	tcpip_initialized = TRUE;
}

struct netif * LwIP_Init()
{
  uint8_t macaddress[6] = ETH_MAC_ADDR;
  struct ip_addr ipaddr;
  struct ip_addr netmask;
  struct ip_addr gw;

  /* Create isr for ethernet interrupt */
  TaskType tid;
  tid = Os_Arc_CreateIsr(Eth_Isr,3/*prio*/,"Eth"); \
  Irq_AttachIsr2(tid,NULL,ETH_IRQn); \

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
  tcpip_init(tcpip_init_done, NULL);
  uint32 lockcnt = 0;
  while(tcpip_initialized == FALSE){
  	 lockcnt++;
  	 SLEEP(0);
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

#if LWIP_DHCP
  /* start dhcp search */
  dhcp_start(&netif);
#else
  netif_set_addr(&netif, &ipaddr , &netmask, &gw);
#endif

  /* netif is configured */
  netif_set_up(&netif);

  EnableEthDMAIrq();

  netbios_init();

  return &netif;
}
