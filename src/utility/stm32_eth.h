/**
  ******************************************************************************
  * @file    stm32_eth.h
  * @author  WI6LABS
  * @version V1.0.0
  * @date    24-May-2017
  * @brief   Include stm32_eth source files
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2016 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

#ifndef __STM32_ETH_H__
#define __STM32_ETH_H__

/* Includes ------------------------------------------------------------------*/
#include "stm32_def.h"
#include "lwip/ip_addr.h"
#include "lwip/dhcp.h"
#include "lwip/udp.h"
#include "lwip/tcp.h"
#include "lwip/opt.h"
#include <functional>

/* Exported types ------------------------------------------------------------*/
/* TCP connection state */
typedef enum {
  TCP_NONE = 0,
  TCP_CONNECTED,
  TCP_RECEIVED,
  TCP_SENT,
  TCP_ACCEPTED,
  TCP_CLOSING,
} tcp_client_states;

/* Struct to store received data */
struct pbuf_data {
  struct pbuf *p;     // the packet buffer that was received
  uint16_t available; // number of data
};

/* UDP structure */
struct udp_struct {
  struct udp_pcb *pcb; /* pointer on the current udp_pcb */
  struct pbuf_data data;
  ip_addr_t ip;       // the remote IP address from which the packet was received
  u16_t port;         // the remote port from which the packet was received
  std::function<void()> onDataArrival;
};

/* TCP structure */
struct tcp_struct {
  struct tcp_pcb *pcb;          /* pointer on the current tcp_pcb */
  struct pbuf_data data;
  tcp_client_states state;      /* current connection state */
};

/* Exported constants --------------------------------------------------------*/
/*Static IP ADDRESS: IP_ADDR0.IP_ADDR1.IP_ADDR2.IP_ADDR3 */
#define IP_ADDR0   (uint8_t) 192
#define IP_ADDR1   (uint8_t) 168
#define IP_ADDR2   (uint8_t) 0
#define IP_ADDR3   (uint8_t) 10

/*NETMASK*/
#define NETMASK_ADDR0   (uint8_t) 255
#define NETMASK_ADDR1   (uint8_t) 255
#define NETMASK_ADDR2   (uint8_t) 255
#define NETMASK_ADDR3   (uint8_t) 0

/*Gateway Address*/
#define GW_ADDR0   (uint8_t) 192
#define GW_ADDR1   (uint8_t) 168
#define GW_ADDR2   (uint8_t) 0
#define GW_ADDR3   (uint8_t) 1

/* DHCP process states */
#define DHCP_OFF                   (uint8_t) 0
#define DHCP_START                 (uint8_t) 1
#define DHCP_WAIT_ADDRESS          (uint8_t) 2
#define DHCP_ADDRESS_ASSIGNED      (uint8_t) 3
#define DHCP_TIMEOUT               (uint8_t) 4
#define DHCP_LINK_DOWN             (uint8_t) 5
#define DHCP_ASK_RELEASE           (uint8_t) 6

/* Maximum number of client per server */
#define MAX_CLIENT  32

#ifdef ETH_INPUT_USE_IT
  extern struct netif gnetif;
#endif


/* Exported functions ------------------------------------------------------- */
/**
  * @brief  Initialize the Ethernet interface and the LwIP stack.
  *
  * LwIP initialization and the scheduler timer setup run only on the first
  * call. Every call reconfigures the network interface and runs one scheduler
  * pass.
  *
  * A NULL argument selects the default value:
  * - mac: the MAC address is left unchanged.
  * - ip, gw, netmask: set to zero when LWIP_DHCP is enabled, otherwise to the
  *   compile-time defaults (IP_ADDR*, GW_ADDR*, NETMASK_ADDR*).
  *
  * @param mac MAC address (6 bytes), or NULL to keep the default MAC address.
  * @param ip IPv4 address (4 bytes), or NULL for the default.
  * @param gw Gateway address (4 bytes), or NULL for the default.
  * @param netmask Netmask (4 bytes), or NULL for the default.
  */
void stm32_eth_init(const uint8_t *mac, const uint8_t *ip, const uint8_t *gw, const uint8_t *netmask);
/**
  * @brief  Check whether the Ethernet interface has been initialized.
  *
  * @return 1 if the Ethernet hardware is initialized, 0 otherwise.
  */
uint8_t stm32_eth_is_init(void);
/**
  * @brief  Copy the MAC address of the Ethernet interface.
  *
  * @param mac Receives the MAC address (6 bytes). Nothing is copied if NULL.
  */
void stm32_eth_get_macaddr(uint8_t *mac);
/**
  * @brief  Set the MAC address of the Ethernet interface.
  *
  * The address is stored only before the interface is initialized.
  *
  * @param mac MAC address (6 bytes). Ignored if NULL.
  */
void stm32_eth_set_macaddr(const uint8_t *mac);
/**
  * @brief  Check whether the Ethernet link is up.
  *
  * @return Non-zero if the link is up, 0 otherwise.
  */
uint8_t stm32_eth_link_up(void);
/**
  * @brief  Run one LwIP scheduler pass.
  *
  * Reads received packets (unless ETH_INPUT_USE_IT is defined), checks the
  * link status, handles LwIP timeouts and runs the periodic DHCP processing.
  */
void stm32_eth_scheduler(void);

/**
  * @brief  Notify the user about the network interface status.
  *
  * If the interface is down and DHCP was started by the user, sets the DHCP
  * state to DHCP_LINK_DOWN.
  *
  * @param netif Network interface to check.
  */
void User_notification(struct netif *netif);

#if LWIP_DHCP
  /**
    * @brief  Run the DHCP state machine for the network interface.
    *
    * @param netif Network interface to process.
    */
  void stm32_DHCP_process(struct netif *netif);
  /**
    * @brief  Run the DHCP state machine at the DHCP fine timer period.
    *
    * Calls stm32_DHCP_process() when at least DHCP_FINE_TIMER_MSECS ms have
    * elapsed since the last run.
    *
    * @param netif Network interface to configure with DHCP.
    */
  void stm32_DHCP_Periodic_Handle(struct netif *netif);
  /**
    * @brief  Send a DHCP INFORM message for the network interface.
    *
    * Used when the interface address is configured manually.
    */
  void stm32_DHCP_manual_config(void);
  /**
    * @brief  Return the DHCP lease state.
    *
    * @return 2 when the lease is renewing, 4 when it is rebinding, 0 otherwise.
    */
  uint8_t stm32_get_DHCP_lease_state(void);
  /**
    * @brief  Set the DHCP state machine state.
    *
    * @param state New DHCP state (for example DHCP_START, DHCP_ASK_RELEASE
    *              or DHCP_OFF).
    */
  void stm32_set_DHCP_state(uint8_t state);
  /**
    * @brief  Return the current DHCP state machine state.
    *
    * @return Current DHCP state (DHCP_OFF, DHCP_START, ...).
    */
  uint8_t stm32_get_DHCP_state(void);
  /**
    * @brief  Return whether DHCP was started by the user.
    *
    * @return 1 if DHCP was started by the user, 0 otherwise.
    */
  uint8_t stm32_dhcp_started(void);
#else
  #error "LWIP_DHCP must be enabled in lwipopts.h"
#endif

#if LWIP_DNS
  /**
    * @brief  Initialize DNS and set the DNS server address.
    *
    * Does nothing when DHCP was started by the user, because DNS is then
    * initialized by DHCP.
    *
    * @param dnsaddr DNS server address (4 bytes).
    */
  void stm32_dns_init(const uint8_t *dnsaddr);
  /**
    * @brief  Resolve a hostname into an IPv4 address.
    *
    * Runs the scheduler while waiting for the lookup, up to
    * TIMEOUT_DNS_REQUEST ms.
    *
    * @param hostname Hostname to resolve.
    * @param ipaddr Receives the resolved address (raw network byte order);
    *               set to 0 on failure or timeout.
    * @return 1 on success, -1 on timeout, -2 if the result is 0, -4 on an
    *         argument or other error.
    */
  int8_t stm32_dns_gethostbyname(const char *hostname, uint32_t *ipaddr);
#else
  #error "LWIP_DNS must be enabled in lwipopts.h"
#endif

#if LWIP_UDP
/**
  * @brief  Callback invoked when a UDP packet is received.
  *
  * If arg matches pcb, stores the packet, the remote address and port, and
  * calls onDataArrival when set. Otherwise the packet is freed.
  *
  * @param arg Pointer to a udp_struct.
  * @param pcb Control block that received the packet.
  * @param p Received packet buffer.
  * @param addr Remote IP address the packet came from.
  * @param port Remote port the packet came from.
  */
void udp_receive_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                          const ip_addr_t *addr, u16_t port);
#else
#error "LWIP_UDP must be enabled in lwipopts.h"
#endif

/**
  * @brief  Return the IPv4 address of the network interface.
  *
  * @return Address as a uint32_t (raw network byte order).
  */
uint32_t stm32_eth_get_ipaddr(void);
/**
  * @brief  Return the gateway address of the network interface.
  *
  * @return Gateway address as a uint32_t (raw network byte order).
  */
uint32_t stm32_eth_get_gwaddr(void);
/**
  * @brief  Return the netmask of the network interface.
  *
  * @return Netmask as a uint32_t (raw network byte order).
  */
uint32_t stm32_eth_get_netmaskaddr(void);
/**
  * @brief  Return the first DNS server address.
  *
  * @return DNS server address (server 0) as a uint32_t (raw network byte
  *         order).
  */
uint32_t stm32_eth_get_dnsaddr(void);
/**
  * @brief  Return the address of the DHCP server.
  *
  * @return DHCP server address as a uint32_t (raw network byte order).
  */
uint32_t stm32_eth_get_dhcpaddr(void);

/**
  * @brief  Allocate a pbuf holding the given data.
  *
  * If p is NULL, allocates a new pbuf and copies buffer into it. Otherwise,
  * allocates a larger pbuf, copies the contents of p and appends buffer, then
  * frees p. On failure, returns NULL and leaves p unchanged.
  *
  * @param p Existing pbuf to extend, or NULL to create a new one.
  * @param buffer Data to store.
  * @param size Number of bytes to store from buffer.
  * @return Pointer to the pbuf holding the data, or NULL on failure.
  */
struct pbuf *stm32_new_data(struct pbuf *p, const uint8_t *buffer, size_t size);
/**
  * @brief  Free a pbuf chain.
  *
  * @param p Pbuf to free; NULL is ignored.
  * @return Always NULL.
  */
struct pbuf *stm32_free_data(struct pbuf *p);
/**
  * @brief  Copy received data from a pbuf_data into a user buffer.
  *
  * Frees the pbufs that are fully consumed.
  *
  * @param data Source of the received data.
  * @param buffer Destination buffer.
  * @param size Maximum number of bytes to copy.
  * @return Number of bytes copied, or 0 on invalid arguments or when no data
  *         is available.
  */
uint16_t stm32_get_data(struct pbuf_data *data, uint8_t *buffer, size_t size);

/**
  * @brief  Convert an IPv4 address from 4 bytes into an ip_addr_t.
  *
  * @param ipu8 Address bytes (4 bytes).
  * @param ipaddr Receives the converted address.
  * @return Pointer to ipaddr.
  */
ip_addr_t *u8_to_ip_addr(uint8_t *ipu8, ip_addr_t *ipaddr);
/**
  * @brief  Convert an ip_addr_t IPv4 address into a uint32_t.
  *
  * @param ipaddr Address to convert.
  * @return The address as a uint32_t (raw network byte order).
  */
uint32_t ip_addr_to_u32(ip_addr_t *ipaddr);

#if LWIP_TCP
  /**
    * @brief  Callback invoked when a TCP connection is established.
    *
    * On success for the matching connection, sets the state to TCP_CONNECTED
    * and installs the recv, sent and err callbacks. On mismatch, closes the
    * connection and returns ERR_ARG. On a non-ERR_OK error, closes the
    * connection and returns that error.
    *
    * @param arg User argument, a struct tcp_struct pointer.
    * @param tpcb Connection control block.
    * @param err ERR_OK when the connection was established correctly.
    * @return ERR_OK on success, ERR_ARG on mismatch, or err otherwise.
    */
  err_t tcp_connected_callback(void *arg, struct tcp_pcb *tpcb, err_t err);
  /**
    * @brief  Callback invoked when a TCP connection is accepted on a listening
    *         port.
    *
    * Allocates a tcp_struct for the new connection and stores it in the first
    * free slot of the client array passed as arg (up to MAX_CLIENT entries).
    * On failure, closes the connection and returns ERR_MEM. If err is not
    * ERR_OK, closes newpcb and returns ERR_ARG.
    *
    * @param arg User argument, a pointer to an array of struct tcp_struct
    *            pointers.
    * @param newpcb Control block of the newly created connection.
    * @param err ERR_OK when the connection was accepted correctly.
    * @return ERR_OK on success, ERR_MEM on allocation or slot failure, or
    *         ERR_ARG on error.
    */
  err_t tcp_accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err);
  /**
    * @brief  Close a TCP connection and detach its callbacks.
    *
    * Removes the callbacks from tpcb, closes it (aborts it if the close
    * fails), clears tcp->pcb and sets the state to TCP_CLOSING.
    *
    * @param tpcb Connection control block to close.
    * @param tcp Connection structure to update.
    */
  void tcp_connection_close(struct tcp_pcb *tpcb, struct tcp_struct *tcp);
#else
  #error "LWIP_TCP must be enabled in lwipopts.h"
#endif

#endif /* __STM32_ETH_H__ */
