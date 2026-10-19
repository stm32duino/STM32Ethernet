/**
  ******************************************************************************
  * @file    ethernetif.h
  * @author  MCD Application Team & Wi6Labs
  * @version V1.5.0
  * @date    20-June-2017
  * @brief   Ethernet interface header file.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics International N.V.
  * All rights reserved.</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice,
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other
  *    contributors to this software may be used to endorse or promote products
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under
  *    this license is void and will automatically terminate your rights under
  *    this license.
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

#ifndef __ETHERNETIF_H__
#define __ETHERNETIF_H__
#ifdef __cplusplus
extern "C" {
#endif
#include "lwip/err.h"
#include "lwip/netif.h"
/* Exported types ------------------------------------------------------------*/
/**
  * @brief  Check whether the Ethernet hardware has been initialized.
  *
  * @return 1 if initialized, 0 otherwise.
  */
uint8_t ethernetif_is_init(void);
/**
  * @brief  LwIP network interface initialization callback.
  *
  * Sets the interface name, the output functions and initializes the Ethernet
  * hardware. Pass it to netif_add().
  *
  * @param netif The LwIP network interface to initialize.
  * @return ERR_OK.
  */
err_t ethernetif_init(struct netif *netif);
/**
  * @brief  Read one received frame from the MAC and pass it to LwIP.
  *
  * Does nothing when no frame is available. If netif->input() fails, the frame
  * is dropped.
  *
  * @param netif The LwIP network interface.
  */
void ethernetif_input(struct netif *netif);
/**
  * @brief  Poll the PHY and update the LwIP link state of the interface.
  *
  * The link is set down when the PHY reports a link-change interrupt, and up
  * when the PHY reports a linked state. With LWIP_IGMP, IGMP is started on the
  * first link-up.
  *
  * @param netif The LwIP network interface.
  */
void ethernetif_set_link(struct netif *netif);
/**
  * @brief  Apply the PHY auto-negotiation result to the MAC.
  *
  * Restarts the MAC with the negotiated duplex and speed, or stops the MAC
  * when the link is down. If auto-negotiation is disabled or has not completed,
  * the configured duplex and speed are used. Calls
  * ethernetif_notify_conn_changed() at the end. Call it when the link status
  * changes.
  *
  * @param netif The LwIP network interface.
  */
void ethernetif_update_config(struct netif *netif);
/**
  * @brief  Notify the user that the link status has changed.
  *
  * Weak function. The default implementation does nothing, and user code can
  * override it.
  *
  * @param netif The LwIP network interface. Not used by the default
  *              implementation.
  */
void ethernetif_notify_conn_changed(struct netif *netif);

/**
  * @brief  Set the MAC address used by the driver.
  *
  * Call it before ethernetif_init(). The address is copied only while the
  * driver is not initialized.
  *
  * @param mac 6-byte MAC address. Ignored if NULL.
  */
void ethernetif_set_mac_addr(const uint8_t *mac);
/**
  * @brief  Copy the MAC address used by the driver.
  *
  * @param mac Buffer of at least 6 bytes that receives the address. Nothing is
  *            copied if NULL.
  */
void ethernetif_get_mac_addr(uint8_t *mac);

#if LWIP_IGMP
/**
  * @brief  LwIP IGMP MAC filter callback.
  *
  * Adds the MAC address of a multicast IPv4 group to the MAC hash filter.
  *
  * @param netif The LwIP network interface. Not used by this implementation.
  * @param ip4_addr Multicast group IPv4 address.
  * @param action Filter action. Not used by this implementation.
  * @return 0 always.
  */
err_t igmp_mac_filter(struct netif *netif, const ip4_addr_t *ip4_addr, netif_mac_filter_action action);
/**
  * @brief  Add a multicast MAC address to the MAC hash table registers.
  *
  * @param mac 6-byte multicast MAC address.
  */
void register_multicast_address(const uint8_t *mac);
#endif

#ifdef __cplusplus
}
#endif

#endif
