#include <fiwix/mm.h>

/* Use the existing allocator. */
#define MEM_CUSTOM_ALLOCATOR 1
#define MEM_CUSTOM_FREE(x) kfree((unsigned int)x)
#define MEM_CUSTOM_MALLOC(x) (void *)kmalloc(x)
#define MEM_CUSTOM_CALLOC my_calloc

#define memcpy memcpy_b
#define memset memset_b
#define FD_ZERO	__FD_ZERO
#define FD_SET __FD_SET
#define FD_ISSET __FD_ISSET

#define ptrdiff_t unsigned int

/* Enable parts of lwIP */
#define LWIP_ARP 1
#define LWIP_ETHERNET 1
#define LWIP_IPV4 1
#define IP_FORWARD 1
#define LWIP_ICMP 1
#define LWIP_DHCP 1
#define LWIP_DNS 1
#define LWIP_UDP 1
#define LWIP_TCP 1
#define LWIP_NETIF_API 1
#define LWIP_SOCKET 1
#define LWIP_IPV6 0
#define LWIP_IPV6_DHCP6 0
#define LWIP_RAW 1

#define SA_FAMILY_T_DEFINED 1

/* new defines */
#define STRUCT_SOCKADDR_DEFINED 1
#define STRUCT_IOVEC_DEFINED 1
#define ADDITIONAL_SO_OPTIONS_DEFINED 1
#define FLAGS_FOR_SEND_RECV_DEFINED 1
#define LWIP_TIMEVAL_PRIVATE 0


/* have a loopback interface */
/*
 * At some point, this should be removed or interfaced with the kernel
 * network API.
 */
#define LWIP_NETIF_LOOPBACK 1


/* Fiwix does not provide a mutex datastructure. */
#define LWIP_COMPAT_MUTEX 1
#define LWIP_COMPAT_MUTEX_ALLOWED 1
