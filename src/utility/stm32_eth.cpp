/**
  ******************************************************************************
  * @file    stm32_eth.cpp
  * @author  WI6LABS
  * @version V1.0.0
  * @date    24-May-2017
  * @brief   This file implements Ethernet network interface drivers for
  *          Arduino STM32Ethernet library.
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

#include "Arduino.h"
#include "stm32_eth.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/timeouts.h"
#include "netif/ethernet.h"
#include "ethernetif.h"
#include "lwip/dhcp.h"
#include "lwip/prot/dhcp.h"
#include "lwip/dns.h"

/* Check ethernet link status every seconds */
#define TIME_CHECK_ETH_LINK_STATE 500U

/* Timeout for DNS request */
#define TIMEOUT_DNS_REQUEST 10000U

/* Maximum number of retries for DHCP request */
#define MAX_DHCP_TRIES  4

/*
 * Defined a default timer used to call ethernet scheduler at regular interval
 * Could be redefined in the variant
 * Note: This is planned to extend Timer API's of the Arduino STM32 core
 *       They could be used for this library when available
 */
#ifndef DEFAULT_ETHERNET_TIMER
  #define DEFAULT_ETHERNET_TIMER  TIM14
  #warning "Default timer used to call ethernet scheduler at regular interval: TIM14"
#endif

/* Interrupt priority */
#ifndef ETH_TIM_IRQ_PRIO
  #define ETH_TIM_IRQ_PRIO       15 // Warning: it should be lower prio (higher value) than Systick
#endif
#ifndef ETH_TIM_IRQ_SUBPRIO
  #define ETH_TIM_IRQ_SUBPRIO    0
#endif
/* Ethernet configuration: user parameters */
struct stm32_eth_config {
  ip_addr_t ipaddr;
  ip_addr_t netmask;
  ip_addr_t gw;
};

/* Use to give user parameters to netif configuration */
static struct stm32_eth_config gconfig;

/* Netif global configuration structure */
struct netif gnetif;

/* DHCP periodic timer */
static uint32_t DHCPfineTimer = 0;

/* DHCP current state */
__IO uint8_t DHCP_state = DHCP_OFF;

/* Set to 1 if user use DHCP to obtain network addresses */
static uint8_t DHCP_Started_by_user = 0;

/* Ethernet link status periodic timer */
static uint32_t gEhtLinkTickStart = 0;

#if !defined(STM32_CORE_VERSION) || (STM32_CORE_VERSION  <= 0x01060100)
  /* Handler for stimer */
  static stimer_t TimHandle;
#else
  HardwareTimer *EthTim = NULL;
#endif

/*************************** Function prototype *******************************/
static void Netif_Config(void);
static err_t tcp_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static err_t tcp_sent_callback(void *arg, struct tcp_pcb *tpcb, u16_t len);
static void tcp_err_callback(void *arg, err_t err);
static void TIM_scheduler_Config(void);
#if defined(STM32_CORE_VERSION) && (STM32_CORE_VERSION  > 0x01060100)
  void _stm32_eth_scheduler(void);
#endif

/**
  * @brief  Configure the network interface.
  *
  * Removes and re-adds gnetif using the addresses in gconfig, makes it the
  * default interface, sets it up or down according to the current link state
  * and, when LWIP_NETIF_LINK_CALLBACK is enabled, registers the link callback.
  */
static void Netif_Config(void)
{
  netif_remove(&gnetif);
  /* Add the network interface */
  netif_add(&gnetif, &(gconfig.ipaddr), &(gconfig.netmask), &(gconfig.gw), NULL, &ethernetif_init, &ethernet_input);

  /* Registers the default network interface */
  netif_set_default(&gnetif);

  if (netif_is_link_up(&gnetif)) {
    /* When the netif is fully configured this function must be called */
    netif_set_up(&gnetif);
  } else {
    /* When the netif link is down this function must be called */
    netif_set_down(&gnetif);
  }

#if LWIP_NETIF_LINK_CALLBACK
  /* Set the link callback function, this function is called on change of link status */
  netif_set_link_callback(&gnetif, ethernetif_update_config);
#endif /* LWIP_NETIF_LINK_CALLBACK */
}

/**
  * @brief  Scheduler callback, called by the timer interrupt every 1 ms.
  *
  * Forwards the call to _stm32_eth_scheduler(). The timer handle argument is
  * present only for STM32 core versions up to 0x01080000 and is unused.
  *
  * @param htim Timer handle (stimer_t or HardwareTimer, depending on the
  *             STM32 core version).
  */
#if !defined(STM32_CORE_VERSION) || (STM32_CORE_VERSION  <= 0x01060100)
  static void scheduler_callback(stimer_t *htim)
#elif (STM32_CORE_VERSION  <= 0x01080000)
  static void scheduler_callback(HardwareTimer *htim)
#else
  static void scheduler_callback(void)
#endif
{
#if (STM32_CORE_VERSION  <= 0x01080000)
  UNUSED(htim);
#endif
  _stm32_eth_scheduler();
}

#if !defined(STM32_CORE_VERSION) || (STM32_CORE_VERSION  <= 0x01060100)
/**
  * @brief  Enable the timer used to call ethernet scheduler function at regular
  *         interval.
  *
  * Uses the stimer API: sets DEFAULT_ETHERNET_TIMER, configures a 1 ms period
  * and attaches scheduler_callback() as its interrupt handler.
  */
static void TIM_scheduler_Config(void)
{
  /* Set TIMx instance. */
  TimHandle.timer = DEFAULT_ETHERNET_TIMER;
  /* Timer set to 1ms */
  TimerHandleInit(&TimHandle, (uint16_t)(1000 - 1), ((uint32_t)(getTimerClkFreq(DEFAULT_ETHERNET_TIMER) / (1000000)) - 1));
  attachIntHandle(&TimHandle, scheduler_callback);
}
#else
/**
  * @brief  Enable the timer used to call ethernet scheduler function at regular
  *         interval.
  *
  * Creates a HardwareTimer on DEFAULT_ETHERNET_TIMER with the ETH_TIM_IRQ_PRIO
  * and ETH_TIM_IRQ_SUBPRIO interrupt priorities, sets a 1 ms overflow, attaches
  * scheduler_callback() and starts the timer.
  */
static void TIM_scheduler_Config(void)
{
  /* Configure HardwareTimer */
  EthTim = new HardwareTimer(DEFAULT_ETHERNET_TIMER);
  EthTim->setInterruptPriority(ETH_TIM_IRQ_PRIO, ETH_TIM_IRQ_SUBPRIO);
  EthTim->setMode(1, TIMER_OUTPUT_COMPARE);

  /* Timer set to 1ms */
  EthTim->setOverflow(1000, MICROSEC_FORMAT);
  EthTim->attachInterrupt(scheduler_callback);
  EthTim->resume();
}
#endif

void stm32_eth_init(const uint8_t *mac, const uint8_t *ip, const uint8_t *gw, const uint8_t *netmask)
{
  static uint8_t initDone = 0;

  if (!initDone) {
    /* Initialize the LwIP stack */
    lwip_init();
  }

  if (mac != NULL) {
    ethernetif_set_mac_addr(mac);
  } // else default value is used: MAC_ADDR0 ... MAC_ADDR5

  if (ip != NULL) {
    IP_ADDR4(&(gconfig.ipaddr), ip[0], ip[1], ip[2], ip[3]);
  } else {
#if LWIP_DHCP
    ip_addr_set_zero_ip4(&(gconfig.ipaddr));
#else
    IP_ADDR4(&(gconfig.ipaddr), IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3);
#endif /* LWIP_DHCP */
  }

  if (gw != NULL) {
    IP_ADDR4(&(gconfig.gw), gw[0], gw[1], gw[2], gw[3]);
  } else {
#if LWIP_DHCP
    ip_addr_set_zero_ip4(&(gconfig.gw));
#else
    IP_ADDR4(&(gconfig.gw), GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
#endif /* LWIP_DHCP */
  }

  if (netmask != NULL) {
    IP_ADDR4(&(gconfig.netmask), netmask[0], netmask[1], netmask[2], netmask[3]);
  } else {
#if LWIP_DHCP
    ip_addr_set_zero_ip4(&(gconfig.netmask));
#else
    IP_ADDR4(&(gconfig.netmask), NETMASK_ADDR0, NETMASK_ADDR1, NETMASK_ADDR2, NETMASK_ADDR3);
#endif /* LWIP_DHCP */
  }

  /* Configure the Network interface */
  Netif_Config();

  if (!initDone) {
    // stm32_eth_scheduler() will be called every 1ms.
    TIM_scheduler_Config();
    initDone = 1;
  }

  /* Reset DHCP if used */
  User_notification(&gnetif);

  /* Update LwIP stack */
  stm32_eth_scheduler();
}

uint8_t stm32_eth_is_init(void)
{
  return ethernetif_is_init();
}

void stm32_eth_set_macaddr(const uint8_t *mac)
{
  ethernetif_set_mac_addr(mac);
}
void stm32_eth_get_macaddr(uint8_t *mac)
{
  return ethernetif_get_mac_addr(mac);
}

uint8_t stm32_eth_link_up(void)
{
  return netif_is_link_up(&gnetif);
}

#if defined(STM32_CORE_VERSION) && (STM32_CORE_VERSION  > 0x01060100)
/* refresh() generates a timer update event, which runs _stm32_eth_scheduler()
   from the timer interrupt. Does nothing until the timer has been created. */
void stm32_eth_scheduler(void)
{
  if (EthTim != NULL) {
    EthTim->refresh();
  }
}

/**
  * @brief  Run one LwIP scheduler pass. Called only from the timer callback.
  *
  * Reads received packets (unless ETH_INPUT_USE_IT is defined), checks the
  * link status every TIME_CHECK_ETH_LINK_STATE ms, handles LwIP timeouts and
  * runs the periodic DHCP processing.
  */
void _stm32_eth_scheduler(void)
#else
void stm32_eth_scheduler(void)
#endif
{
  /* Read a received packet from the Ethernet buffers and send it
  to the lwIP for handling */
#ifndef ETH_INPUT_USE_IT
  ethernetif_input(&gnetif);
#endif /* ETH_INPUT_USE_IT */

  /* Check ethernet link status */
  if ((HAL_GetTick() - gEhtLinkTickStart) >= TIME_CHECK_ETH_LINK_STATE) {
    ethernetif_set_link(&gnetif);
    gEhtLinkTickStart = HAL_GetTick();
  }

  /* Handle LwIP timeouts */
  sys_check_timeouts();

#if LWIP_DHCP
  stm32_DHCP_Periodic_Handle(&gnetif);
#endif /* LWIP_DHCP */
}

#if LWIP_DHCP

uint8_t stm32_dhcp_started(void)
{
  return DHCP_Started_by_user;
}

/* With the link up, DHCP_state is advanced as follows:
   DHCP_START starts DHCP and moves to DHCP_WAIT_ADDRESS.
   DHCP_WAIT_ADDRESS moves to DHCP_ADDRESS_ASSIGNED once an address is supplied.
   Otherwise, after more than MAX_DHCP_TRIES attempts it moves to DHCP_TIMEOUT
   and stops DHCP.
   DHCP_ASK_RELEASE releases the lease, stops DHCP and moves to DHCP_OFF.
   DHCP_LINK_DOWN stops DHCP and moves to DHCP_OFF.
   With the link down, DHCP_state is set to DHCP_OFF. */
void stm32_DHCP_process(struct netif *netif)
{
  struct dhcp *dhcp;

  if (netif_is_link_up(netif)) {
    switch (DHCP_state) {
      case DHCP_START: {
          ip_addr_set_zero_ip4(&netif->ip_addr);
          ip_addr_set_zero_ip4(&netif->netmask);
          ip_addr_set_zero_ip4(&netif->gw);
          DHCP_state = DHCP_WAIT_ADDRESS;
          dhcp_start(netif);
          DHCP_Started_by_user = 1;
        }
        break;

      case DHCP_WAIT_ADDRESS: {
          if (dhcp_supplied_address(netif)) {
            DHCP_state = DHCP_ADDRESS_ASSIGNED;
          } else {
            dhcp = (struct dhcp *)netif_get_client_data(netif, LWIP_NETIF_CLIENT_DATA_INDEX_DHCP);

            /* DHCP timeout */
            if (dhcp->tries > MAX_DHCP_TRIES) {
              DHCP_state = DHCP_TIMEOUT;

              // If DHCP address not bind, keep DHCP stopped
              DHCP_Started_by_user = 0;

              /* Stop DHCP */
              dhcp_stop(netif);
            }
          }
        }
        break;
      case DHCP_ASK_RELEASE: {
          /* Force release */
          dhcp_release(netif);
          dhcp_stop(netif);
          DHCP_state = DHCP_OFF;
        }
        break;
      case DHCP_LINK_DOWN: {
          /* Stop DHCP */
          dhcp_stop(netif);
          DHCP_state = DHCP_OFF;
        }
        break;
      default: break;
    }
  } else {
    DHCP_state = DHCP_OFF;
  }
}

void stm32_DHCP_Periodic_Handle(struct netif *netif)
{
  /* Fine DHCP periodic process every 500ms */
  if (HAL_GetTick() - DHCPfineTimer >= DHCP_FINE_TIMER_MSECS) {
    DHCPfineTimer =  HAL_GetTick();
    /* process DHCP state machine */
    stm32_DHCP_process(netif);
  }
}

void stm32_DHCP_manual_config(void)
{
  dhcp_inform(&gnetif);
}

/* Adapted from the Arduino Ethernet library. The DHCP client data of gnetif
   is read without a NULL check. */
uint8_t stm32_get_DHCP_lease_state(void)
{
  uint8_t res = 0;
  struct dhcp *dhcp = (struct dhcp *)netif_get_client_data(&gnetif, LWIP_NETIF_CLIENT_DATA_INDEX_DHCP);

  if (dhcp->state == DHCP_STATE_RENEWING) {
    res = 2;
  } else if (dhcp->state == DHCP_STATE_REBINDING) {
    res = 4;
  }

  return res;
}

void stm32_set_DHCP_state(uint8_t state)
{
  DHCP_state = state;
}

uint8_t stm32_get_DHCP_state(void)
{
  return DHCP_state;
}

#endif /* LWIP_DHCP */

uint32_t stm32_eth_get_ipaddr(void)
{
  return ip4_addr_get_u32(&(gnetif.ip_addr));
}

uint32_t stm32_eth_get_gwaddr(void)
{
  return ip4_addr_get_u32(&(gnetif.gw));
}

uint32_t stm32_eth_get_netmaskaddr(void)
{
  return ip4_addr_get_u32(&(gnetif.netmask));
}

uint32_t stm32_eth_get_dnsaddr(void)
{
  const ip_addr_t *tmp = dns_getserver(0);
  return ip4_addr_get_u32(tmp);
}

/* The DHCP client data of gnetif is read without a NULL check. */
uint32_t stm32_eth_get_dhcpaddr(void)
{
  struct dhcp *dhcp = (struct dhcp *)netif_get_client_data(&gnetif, LWIP_NETIF_CLIENT_DATA_INDEX_DHCP);
  return ip4_addr_get_u32(&(dhcp->server_ip_addr));
}

#if LWIP_NETIF_LINK_CALLBACK

/* Overrides the weak default in ethernetif.cpp. When the link is up, restarts
   DHCP if the user started it and sets the interface up. When the link is down,
   sets DHCP_LINK_DOWN if the user started DHCP and sets the interface down. */
void ethernetif_notify_conn_changed(struct netif *netif)
{
  if (netif_is_link_up(netif)) {
    /* Update DHCP state machine if DHCP used */
    if (DHCP_Started_by_user == 1) {
      DHCP_state = DHCP_START;
    }

    /* When the netif is fully configured this function must be called.*/
    netif_set_up(netif);
  } else {
    /* Update DHCP state machine if DHCP used */
    if (DHCP_Started_by_user == 1) {
      DHCP_state = DHCP_LINK_DOWN;
    }

    /*  When the netif link is down this function must be called.*/
    netif_set_down(netif);
  }
}

#endif /* LWIP_NETIF_LINK_CALLBACK */

void User_notification(struct netif *netif)
{
  if (netif_is_up(netif)) {
    //Link up
  } else {
    /* Update DHCP state machine */
    if (DHCP_Started_by_user == 1) {
      DHCP_state = DHCP_LINK_DOWN;
    }
  }
}

#if LWIP_DNS

void stm32_dns_init(const uint8_t *dnsaddr)
{
  ip_addr_t ip;

  /* DNS initialized by DHCP when call dhcp_start() */
  if (!stm32_dhcp_started()) {
    dns_init();
    IP_ADDR4(&ip, dnsaddr[0], dnsaddr[1], dnsaddr[2], dnsaddr[3]);
    dns_setserver(0, &ip);
  }
}

/**
  * @brief  Callback invoked when a DNS lookup completes.
  *
  * Stores the resolved IPv4 address in the uint32_t pointed to by
  * callback_arg, or 0 if the name could not be resolved.
  *
  * @param name Hostname that was looked up (unused).
  * @param ipaddr Resolved address, or NULL if the lookup failed.
  * @param callback_arg Pointer to a uint32_t that receives the address.
  */
void dns_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg)
{
  UNUSED(name);

  if (ipaddr != NULL) {
    *((uint32_t *)callback_arg) = ip4_addr_get_u32(ipaddr);
  } else {
    *((uint32_t *)callback_arg) = 0;
  }
}

int8_t stm32_dns_gethostbyname(const char *hostname, uint32_t *ipaddr)
{
  ip_addr_t iphost;
  err_t err;
  uint32_t tickstart = 0;
  int8_t ret = 0;

  *ipaddr = 0;
  err = dns_gethostbyname(hostname, &iphost, &dns_callback, ipaddr);

  switch (err) {
    case ERR_OK:
      *ipaddr = ip4_addr_get_u32(&iphost);
      ret = 1;
      break;

    case ERR_INPROGRESS:
      tickstart = HAL_GetTick();
      while (*ipaddr == 0) {
        stm32_eth_scheduler();
        if ((HAL_GetTick() - tickstart) >= TIMEOUT_DNS_REQUEST) {
          ret = -1;
          break;
        }
      }

      if (ret == 0) {
        if (*ipaddr == 0) {
          ret = -2;
        } else {
          ret = 1;
        }
      }
      break;

    case ERR_ARG:
      ret = -4;
      break;

    default:
      ret = -4;
      break;
  }

  return ret;
}

#endif /* LWIP_DNS */

ip_addr_t *u8_to_ip_addr(uint8_t *ipu8, ip_addr_t *ipaddr)
{
  IP_ADDR4(ipaddr, ipu8[0], ipu8[1], ipu8[2], ipu8[3]);
  return ipaddr;
}

uint32_t ip_addr_to_u32(ip_addr_t *ipaddr)
{
  return ip4_addr_get_u32(ipaddr);
}

struct pbuf *stm32_new_data(struct pbuf *p, const uint8_t *buffer, size_t size)
{
  // Allocate memory if pbuf doesn't exit yet.
  if (p == NULL) {
    p = pbuf_alloc(PBUF_TRANSPORT, size, PBUF_RAM);

    if (p != NULL) {
      // Copy data inside pbuf
      if (ERR_OK == pbuf_take(p, (uint8_t *)buffer, size)) {
        return p;
      } else {
        pbuf_free(p);
      }
    }
  }
  // If pbuf allocated, grow the size of pbuf and add new data
  // NOTE: pbuf_realloc can't be used to grow the size of pbuf
  else {
    struct pbuf *q = pbuf_alloc(PBUF_TRANSPORT, size + p->tot_len, PBUF_RAM);

    if (q != NULL) {
      if (ERR_OK == pbuf_copy(q, p)) {
        if (ERR_OK == pbuf_take_at(q, (uint8_t *)buffer, size, p->tot_len)) {
          pbuf_free(p);
          p = q;
          return p;
        }
      }

      pbuf_free(q);
    }
  }

  return 0;
}

/* pbuf_free() returns 0 while the pbuf is still referenced, so it is called
   until the pbuf is released. */
struct pbuf *stm32_free_data(struct pbuf *p)
{
  uint16_t n;

  if (p != NULL) {
    do {
      n = pbuf_free(p);
    } while (n == 0);
  }

  return NULL;
}

/* Each copied byte decrements data->available. A pbuf that is fully read is
   freed and data->p advances to the next one. The chain is released once
   data->available reaches 0. */
uint16_t stm32_get_data(struct pbuf_data *data, uint8_t *buffer, size_t size)
{
  uint16_t i;
  uint16_t offset;
  uint16_t nb;
  struct pbuf *ptr;

  if ((data->p == NULL) || (buffer == NULL) || (size == 0) ||
      (data->available == 0) || (data->available > data->p->tot_len)) {
    return 0;
  }

  nb = 0;

  while ((nb < size) && (data->p != NULL) && (data->available > 0)) {
    ptr = data->p;
    offset = ptr->tot_len - data->available;

    /* Get data from p */
    for (i = 0; (nb < size) && ((offset + i) < ptr->len) && (data->available > 0); i++) {
      buffer[nb] = pbuf_get_at(ptr, offset + i);
      nb++;
      data->available--;
    }

    if (nb < size) {
      /* continue with next pbuf in chain (if any) */
      data->p = ptr->next;

      if (data->p != NULL) {
        /* increment reference count for p */
        pbuf_ref(data->p);
      }

      /* chop first pbuf from chain */
      ptr = stm32_free_data(ptr);
    }
  }

  if (data->available == 0) {
    data->p = stm32_free_data(data->p);
  }

  return nb;
}

#if LWIP_UDP

void udp_receive_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                          const ip_addr_t *addr, u16_t port)
{
  struct udp_struct *udp_arg = (struct udp_struct *)arg;

  /* Send data to the application layer */
  if ((udp_arg != NULL) && (udp_arg->pcb == pcb)) {
    // Free the old p buffer if not read
    if (udp_arg->data.p != NULL) {
      pbuf_free(udp_arg->data.p);
    }

    udp_arg->data.p = p;
    udp_arg->data.available = p->len;

    ip_addr_copy(udp_arg->ip, *addr);
    udp_arg->port = port;

    if (udp_arg->onDataArrival != NULL) {
      udp_arg->onDataArrival();
    }
  } else {
    pbuf_free(p);
  }
}

#endif /* LWIP_UDP */

#if LWIP_TCP

err_t tcp_connected_callback(void *arg, struct tcp_pcb *tpcb, err_t err)
{
  struct tcp_struct *tcp_arg = (struct tcp_struct *)arg;

  if (err == ERR_OK) {
    if ((tcp_arg != NULL) && (tcp_arg->pcb == tpcb)) {
      tcp_arg->state = TCP_CONNECTED;

      /* initialize LwIP tcp_recv callback function */
      tcp_recv(tpcb, tcp_recv_callback);

      /* initialize LwIP tcp_sent callback function */
      tcp_sent(tpcb, tcp_sent_callback);

      /* initialize LwIP tcp_err callback function */
      tcp_err(tpcb, tcp_err_callback);

      return ERR_OK;
    } else {
      /* close connection */
      tcp_connection_close(tpcb, tcp_arg);

      return ERR_ARG;
    }
  } else {
    /* close connection */
    tcp_connection_close(tpcb, tcp_arg);
  }
  return err;
}

err_t tcp_accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err)
{
  err_t ret_err;
  bool accepted = false;
  struct tcp_struct **tcpClient = (struct tcp_struct **)arg;

  /* set priority for the newly accepted tcp connection newpcb */
  tcp_setprio(newpcb, TCP_PRIO_MIN);

  if ((tcpClient != NULL) && (ERR_OK == err)) {
    struct tcp_struct *client = (struct tcp_struct *)mem_malloc(sizeof(struct tcp_struct));

    if (client != NULL) {
      client->state = TCP_ACCEPTED;
      client->pcb = newpcb;
      client->data.p = NULL;
      client->data.available = 0;

      /* Looking for an empty socket */
      for (uint16_t i = 0; i < MAX_CLIENT; i++) {
        if (tcpClient[i] == NULL) {
          tcpClient[i] = client;
          accepted = true;
          break;
        }
      }

      if (accepted) {
        /* pass newly allocated client structure as argument to newpcb */
        tcp_arg(newpcb, client);

        /* initialize lwip tcp_recv callback function for newpcb  */
        tcp_recv(newpcb, tcp_recv_callback);

        /* initialize lwip tcp_err callback function for newpcb  */
        tcp_err(newpcb, tcp_err_callback);

        /* initialize LwIP tcp_sent callback function */
        tcp_sent(newpcb, tcp_sent_callback);

        ret_err = ERR_OK;
      } else {
        /*  close tcp connection */
        tcp_connection_close(newpcb, client);
        mem_free(client);

        /* return memory error */
        ret_err = ERR_MEM;
      }
    } else {
      /*  close tcp connection */
      tcp_connection_close(newpcb, client);
      mem_free(client);

      /* return memory error */
      ret_err = ERR_MEM;
    }
  } else {
    tcp_close(newpcb);
    ret_err = ERR_ARG;
  }
  return ret_err;
}

/**
  * @brief  Callback invoked when data is received on a TCP connection.
  *
  * An empty packet (p == NULL) closes the connection. A packet received with
  * an error is freed and the error is returned. For a connected or accepted
  * connection, acknowledges the data and appends it to the unread data
  * chain. For a closed connection, acknowledges and frees the data.
  *
  * @param arg User argument, a struct tcp_struct pointer.
  * @param tpcb Connection control block.
  * @param p Received packet buffer, or NULL when the peer closed the
  *          connection.
  * @param err Receive error code.
  * @return ERR_OK, or the receive error code.
  */
static err_t tcp_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
  struct tcp_struct *tcp_arg = (struct tcp_struct *)arg;
  err_t ret_err;

  /* if we receive an empty tcp frame from server => close connection */
  if (p == NULL) {
    /* we're done sending, close connection */
    tcp_connection_close(tpcb, tcp_arg);
    ret_err = ERR_OK;
  }
  /* else : a non empty frame was received from echo server but for some reason err != ERR_OK */
  else if (err != ERR_OK) {
    /* free received pbuf*/
    if (p != NULL) {
      pbuf_free(p);
    }
    ret_err = err;
  } else if ((tcp_arg->state == TCP_CONNECTED) || (tcp_arg->state == TCP_ACCEPTED)) {
    /* Acknowledge data reception */
    tcp_recved(tpcb, p->tot_len);

    if (tcp_arg->data.p == NULL) {
      tcp_arg->data.p = p;
    } else {
      pbuf_chain(tcp_arg->data.p, p);
    }

    tcp_arg->data.available += p->len;
    ret_err = ERR_OK;
  }
  /* data received when connection already closed */
  else {
    /* Acknowledge data reception */
    tcp_recved(tpcb, p->tot_len);

    /* free pbuf and do nothing */
    pbuf_free(p);
    ret_err = ERR_OK;
  }
  return ret_err;
}

/**
  * @brief  Callback invoked when an ACK is received for sent TCP data.
  *
  * @param arg User argument, a struct tcp_struct pointer.
  * @param tpcb Connection control block.
  * @param len Length of the acknowledged data (unused).
  * @return ERR_OK if arg refers to tpcb, ERR_ARG otherwise.
  */
static err_t tcp_sent_callback(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
  struct tcp_struct *tcp_arg = (struct tcp_struct *)arg;

  LWIP_UNUSED_ARG(len);

  if ((tcp_arg != NULL) && (tcp_arg->pcb == tpcb)) {
    return ERR_OK;
  }

  return ERR_ARG;
}

/**
  * @brief  Callback invoked when a TCP connection is reset or closed
  *         unexpectedly.
  *
  * On an error other than ERR_OK, clears the connection's pcb and sets its
  * state to TCP_CLOSING.
  *
  * @note The corresponding pcb is already freed when this callback is called.
  *
  * @param arg User argument, a struct tcp_struct pointer.
  * @param err Error code indicating why the pcb was closed:
  *            ERR_ABRT if aborted through tcp_abort or by a TCP timer,
  *            ERR_RST if the connection was reset by the remote host.
  */
static void tcp_err_callback(void *arg, err_t err)
{
  struct tcp_struct *tcp_arg = (struct tcp_struct *)arg;

  if (tcp_arg != NULL) {
    if (ERR_OK != err) {
      tcp_arg->pcb = NULL;
      tcp_arg->state = TCP_CLOSING;
    }
  }
}

void tcp_connection_close(struct tcp_pcb *tpcb, struct tcp_struct *tcp)
{
  /* remove callbacks */
  tcp_recv(tpcb, NULL);
  tcp_sent(tpcb, NULL);
  tcp_poll(tpcb, NULL, 0);
  tcp_err(tpcb, NULL);
  tcp_accept(tpcb, NULL);

  /* close tcp connection */
  if (tcp_close(tpcb) != ERR_OK) {
    tcp_abort(tpcb);
  }

  tcp->pcb = NULL;
  tcp->state = TCP_CLOSING;
}

#endif /* LWIP_TCP */
