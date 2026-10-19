/**
 * @file    STM32Ethernet.h
 * @brief   Main header of the STM32Ethernet library.
 *
 * Declares EthernetClass, the global Ethernet object, and EthernetLinkStatus.
 * It includes the client, server and DHCP headers.
 */
#ifndef ethernet_h
#define ethernet_h

#include <inttypes.h>
#include "IPAddress.h"
#include "EthernetClient.h"
#include "EthernetServer.h"
#include "Dhcp.h"

enum EthernetLinkStatus {
  Unknown,
  LinkON,
  LinkOFF
};

class EthernetClass {
  private:
    IPAddress _dnsServerAddress;
    DhcpClass *_dhcp;

  public:
    /**
     * @brief Initialises Ethernet with the built-in MAC address and gets the
     *        network configuration from a DHCP server.
     *
     * Blocks until a lease is obtained or the timeout expires. On success the
     * DNS server is taken from DHCP.
     *
     * @param timeout Maximum time to wait for the DHCP lease, in milliseconds.
     * @param responseTimeout Unused; kept for compatibility.
     * @return 1 if the DHCP configuration succeeded, 0 if it failed.
     */
    int begin(unsigned long timeout = 60000, unsigned long responseTimeout = 4000);
    /**
     * @brief Returns the Ethernet link status.
     *
     * @return Unknown if the interface is not initialised, LinkON if the link
     *         is up, LinkOFF otherwise.
     */
    EthernetLinkStatus linkStatus();
    /**
     * @brief Initialises Ethernet with a static IP address.
     *
     * Uses the built-in MAC address and a subnet mask of 255.255.255.0.
     *
     * @param local_ip Static IP address of the board.
     */
    void begin(IPAddress local_ip);
    /**
     * @brief Initialises Ethernet with a static IP address and subnet mask.
     *
     * The gateway defaults to the .1 address of the local network.
     *
     * @param local_ip Static IP address of the board.
     * @param subnet Subnet mask.
     */
    void begin(IPAddress local_ip, IPAddress subnet);
    /**
     * @brief Initialises Ethernet with static IP, subnet mask and gateway.
     *
     * The DNS server defaults to the gateway address.
     *
     * @param local_ip Static IP address of the board.
     * @param subnet Subnet mask.
     * @param gateway Default gateway address.
     */
    void begin(IPAddress local_ip, IPAddress subnet, IPAddress gateway);
    /**
     * @brief Initialises Ethernet with a fully static network configuration.
     *
     * Uses the built-in MAC address and informs any local DHCP server of the
     * static configuration to avoid IP conflicts. The DNS server is only
     * stored and returned by dnsServerIP().
     *
     * @param local_ip Static IP address of the board.
     * @param subnet Subnet mask.
     * @param gateway Default gateway address.
     * @param dns_server DNS server address.
     */
    void begin(IPAddress local_ip, IPAddress subnet, IPAddress gateway, IPAddress dns_server);
    /**
     * @brief Initialises Ethernet with the given MAC address and gets the
     *        network configuration from a DHCP server.
     *
     * The MAC address is only applied if the interface is not yet initialised.
     * Blocks until a lease is obtained or the timeout expires. On success the
     * DNS server is taken from DHCP.
     *
     * @param mac_address Six-byte MAC address to use.
     * @param timeout Maximum time to wait for the DHCP lease, in milliseconds.
     * @param responseTimeout Unused; kept for compatibility.
     * @return 1 if the DHCP configuration succeeded, 0 if it failed.
     */
    int begin(uint8_t *mac_address, unsigned long timeout = 60000, unsigned long responseTimeout = 4000);
    /**
     * @brief Initialises Ethernet with a MAC address and a static IP address.
     *
     * Uses a subnet mask of 255.255.255.0. The DNS server and the gateway
     * default to the .1 address of the local network.
     *
     * @param mac_address Six-byte MAC address to use.
     * @param local_ip Static IP address of the board.
     */
    void begin(uint8_t *mac_address, IPAddress local_ip);
    /**
     * @brief Initialises Ethernet with a MAC address, static IP and DNS server.
     *
     * Uses a subnet mask of 255.255.255.0. The gateway defaults to the .1
     * address of the local network.
     *
     * @param mac_address Six-byte MAC address to use.
     * @param local_ip Static IP address of the board.
     * @param dns_server DNS server address.
     */
    void begin(uint8_t *mac_address, IPAddress local_ip, IPAddress dns_server);
    /**
     * @brief Initialises Ethernet with a MAC address, static IP, DNS and gateway.
     *
     * Uses a subnet mask of 255.255.255.0.
     *
     * @param mac_address Six-byte MAC address to use.
     * @param local_ip Static IP address of the board.
     * @param dns_server DNS server address.
     * @param gateway Default gateway address.
     */
    void begin(uint8_t *mac_address, IPAddress local_ip, IPAddress dns_server, IPAddress gateway);
    /**
     * @brief Initialises Ethernet with a MAC address and a fully static
     *        network configuration.
     *
     * Informs any local DHCP server of the static configuration to avoid IP
     * conflicts. The DNS server is only stored and returned by dnsServerIP().
     *
     * @param mac_address Six-byte MAC address to use.
     * @param local_ip Static IP address of the board.
     * @param dns_server DNS server address.
     * @param gateway Default gateway address.
     * @param subnet Subnet mask.
     */
    void begin(uint8_t *mac_address, IPAddress local_ip, IPAddress dns_server, IPAddress gateway, IPAddress subnet);


    /**
     * @brief Checks the DHCP lease and reports the result of a renew or rebind.
     *
     * Does nothing and returns DHCP_CHECK_NONE if DHCP was never started. When
     * a renew or rebind succeeds, the DNS server is updated from DHCP.
     *
     * @return DHCP_CHECK_NONE (0) if nothing happened, DHCP_CHECK_RENEW_FAIL
     *         (1), DHCP_CHECK_RENEW_OK (2), DHCP_CHECK_REBIND_FAIL (3) or
     *         DHCP_CHECK_REBIND_OK (4).
     */
    int maintain();
    /**
     * @brief Services the LwIP stack.
     *
     * Triggers the stack scheduler, which handles packet input, the link
     * check, timeouts and DHCP processing. Useful in long loops where the
     * stack would otherwise not be updated.
     */
    void schedule(void);

    /**
     * @brief Copies the current MAC address of the Ethernet interface.
     *
     * @param mac_address Six-byte buffer that receives the MAC address.
     */
    void MACAddress(uint8_t *mac_address);
    /**
     * @brief Returns the local IP address of the interface.
     *
     * @return The current IP address.
     */
    IPAddress localIP();
    /**
     * @brief Returns the subnet mask of the interface.
     *
     * @return The current subnet mask.
     */
    IPAddress subnetMask();
    /**
     * @brief Returns the default gateway address of the interface.
     *
     * @return The current gateway address.
     */
    IPAddress gatewayIP();
    /**
     * @brief Returns the DNS server address.
     *
     * This is the value stored by begin() or setDnsServerIP().
     *
     * @return The DNS server address.
     */
    IPAddress dnsServerIP();

    /**
     * @brief Sets the MAC address of the Ethernet interface.
     *
     * The address is only applied while the interface is not yet initialised.
     *
     * @param mac_address Six-byte MAC address to use.
     */
    void setMACAddress(const uint8_t *mac_address);
    /**
     * @brief Stores the DNS server address returned by dnsServerIP().
     *
     * Does not reconfigure the LwIP DNS resolver.
     *
     * @param dns_server DNS server address.
     */
    void setDnsServerIP(const IPAddress dns_server);

    friend class EthernetClient;
    friend class EthernetServer;
};

extern EthernetClass Ethernet;

#endif
