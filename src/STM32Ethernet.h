#ifndef ethernet_h
#define ethernet_h

#include <inttypes.h>
#include "IPAddress.h"
#include "EthernetClient.h"
#include "EthernetServer.h"
#include "Dhcp.h"

class EthernetClass {
private:
  IPAddress _dnsServerAddress;
  DhcpClass* _dhcp;
public:
  // Initialise the Ethernet with the internal provided MAC address and gain the rest of the
  // configuration through DHCP.
  // Returns 0 if the DHCP configuration failed, and 1 if it succeeded
  int begin(unsigned long timeout = 60000, unsigned long responseTimeout = 4000);
  void begin(IPAddress local_ip);
  void begin(IPAddress local_ip, IPAddress subnet);
  void begin(IPAddress local_ip, IPAddress subnet, IPAddress gateway);
  void begin(IPAddress local_ip, IPAddress subnet, IPAddress gateway, IPAddress dns_server);
  int maintain();
  void schedule(void);

  void macAddress(uint8_t *mac);
  IPAddress localIP();
  IPAddress subnetMask();
  IPAddress gatewayIP();
  IPAddress dnsServerIP();

  friend class EthernetClient;
  friend class EthernetServer;
};

extern EthernetClass Ethernet;

#endif
