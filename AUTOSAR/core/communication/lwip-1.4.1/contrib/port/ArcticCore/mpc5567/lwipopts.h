#define LWIP_AR_MAJOR_VERSION   1
#define LWIP_AR_MINOR_VERSION   0
#define LWIP_AR_PATCH_VERSION   0

#define LWIP_SW_MAJOR_VERSION   1
#define LWIP_SW_MINOR_VERSION   0
#define LWIP_SW_PATCH_VERSION   0

/* New important lwip1.4.1 defines */
#define LWIP_TCPIP_CORE_LOCKING 1
#define LWIP_CHECKSUM_ON_COPY  1

// We want to be notified when the link changes status.
#define LWIP_NETIF_STATUS_CALLBACK 1

#include "LwIp_Cfg.h"
