/**
 * @file    Dhcp.h
 * @brief   DHCP client declarations for the Ethernet library.
 *
 * Declares DhcpClass, which runs the DHCP state machine on top of LwIP, checks
 * lease renewal and rebinding, and returns the addresses assigned by the server.
 */
// DHCP Library v0.3 - April 25, 2009
// Author: Jordan Terrell - blog.jordanterrell.com

#ifndef Dhcp_h
#define Dhcp_h

#include "EthernetUdp.h"

/* DHCP state machine. */
#define STATE_DHCP_STOP       DHCP_OFF
#define STATE_DHCP_START      DHCP_START
#define STATE_DHCP_DISCOVER   DHCP_WAIT_ADDRESS
#define STATE_DHCP_REQUEST    0
#define STATE_DHCP_LEASED     DHCP_ADDRESS_ASSIGNED
#define STATE_DHCP_REREQUEST  0
#define STATE_DHCP_RELEASE    DHCP_ASK_RELEASE

#define DHCP_CHECK_NONE         (0)
#define DHCP_CHECK_RENEW_FAIL   (1)
#define DHCP_CHECK_RENEW_OK     (2)
#define DHCP_CHECK_REBIND_FAIL  (3)
#define DHCP_CHECK_REBIND_OK    (4)

class DhcpClass {
  private:
    uint8_t  _dhcpMacAddr[6];
    unsigned long _timeout;
    uint8_t _dhcp_lease_state;
    uint8_t _dhcp_state;

    /**
     * @brief Requests a DHCP lease and waits until it is granted or the
     *        timeout expires.
     *
     * If the timeout expires, the lease request is reset.
     *
     * @return 1 if the lease was granted, 0 otherwise.
     */
    int request_DHCP_lease();
    /**
     * @brief Releases the current DHCP lease and updates the DHCP state.
     */
    void reset_DHCP_lease();

  public:
    /**
     * @brief Returns the local IP address assigned by DHCP.
     *
     * @return The local IP address.
     */
    IPAddress getLocalIp();
    /**
     * @brief Returns the subnet mask assigned by DHCP.
     *
     * @return The subnet mask.
     */
    IPAddress getSubnetMask();
    /**
     * @brief Returns the default gateway address assigned by DHCP.
     *
     * @return The gateway address.
     */
    IPAddress getGatewayIp();
    /**
     * @brief Returns the address of the DHCP server.
     *
     * @return The DHCP server address.
     */
    IPAddress getDhcpServerIp();
    /**
     * @brief Returns the DNS server address assigned by DHCP.
     *
     * @return The DNS server address.
     */
    IPAddress getDnsServerIp();

    /**
     * @brief Starts a DHCP lease request.
     *
     * Blocks until a lease is obtained or the timeout expires.
     *
     * @param mac Six-byte MAC address to use. If NULL, the MAC address of the
     *            Ethernet interface is used.
     * @param timeout Maximum time to wait for the lease, in milliseconds.
     * @param responseTimeout Unused; kept for compatibility.
     * @return 1 if the lease was granted, 0 otherwise.
     */
    int beginWithDHCP(uint8_t *mac, unsigned long timeout = 60000, unsigned long responseTimeout = 4000);
    /**
     * @brief Checks the DHCP lease for a renew or rebind result.
     *
     * Runs the Ethernet scheduler on each call. Applications normally call it
     * through Ethernet.maintain(), which should be run regularly.
     *
     * @return DHCP_CHECK_NONE (0) if nothing happened, DHCP_CHECK_RENEW_FAIL
     *         (1), DHCP_CHECK_RENEW_OK (2), DHCP_CHECK_REBIND_FAIL (3) or
     *         DHCP_CHECK_REBIND_OK (4).
     */
    int checkLease();
};

#endif
