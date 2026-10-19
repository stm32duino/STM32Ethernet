/**
 * @file    Dns.h
 * @brief   DNS client declarations for the Ethernet library.
 *
 * Declares DNSClient, which sets the DNS server and resolves host names.
 * BuildRequest() and ProcessResponse() are deprecated and do nothing useful.
 */
// Arduino DNS client for WizNet5100-based Ethernet shield
// (c) Copyright 2009-2010 MCQN Ltd.
// Released under Apache License, version 2.0

#ifndef DNSClient_h
#define DNSClient_h

#include <EthernetUdp.h>

class DNSClient {
  public:
    /**
     * @brief Sets the DNS server used for host name lookups.
     *
     * Passes the server to the LwIP DNS resolver. Has no effect if DHCP has
     * already started.
     *
     * @param aDNSServer DNS server address.
     */
    void begin(const IPAddress &aDNSServer);

    /**
     * @brief Converts a dotted-decimal IP address string to an IP address.
     *
     * @param address IP address string to convert, in the form
     *        "a.b.c.d" with each part in the range 0 to 255.
     * @param result IPAddress that receives the converted address. It may be
     *        partly written even if the conversion fails.
     * @return 1 if the string was converted, 0 otherwise.
     */
    int inet_aton(const char *address, IPAddress &result);

    /**
     * @brief Resolves a host name to an IP address.
     *
     * A numeric address string is converted directly. Otherwise the name is
     * resolved through the DNS server set with begin().
     *
     * @param aHostname Host name or numeric address to resolve.
     * @param aResult IPAddress that receives the resolved address.
     * @return 1 on success. -2 if no valid DNS server is set. Otherwise the
     *         error code of the resolver: -1 on timeout, -2 if no address was
     *         found, or -4 on an LwIP error.
     */
    int getHostByName(const char *aHostname, IPAddress &aResult);

  protected:
    /**
     * @brief Deprecated. Does nothing.
     *
     * @deprecated Do not use.
     * @param aName Unused.
     * @return Always 0.
     */
    uint16_t BuildRequest(const char *aName);
    /**
     * @brief Deprecated. Does nothing.
     *
     * @deprecated Do not use.
     * @param aTimeout Unused.
     * @param aAddress Unused.
     * @return Always -10, cast to uint16_t.
     */
    uint16_t ProcessResponse(uint16_t aTimeout, IPAddress &aAddress);

    IPAddress iDNSServer;
};

#endif
