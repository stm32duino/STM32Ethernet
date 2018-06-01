#include "STM32Ethernet.h"
#include "Dhcp.h"

int EthernetClass::begin(unsigned long timeout, unsigned long responseTimeout)
{
  static DhcpClass s_dhcp;
  _dhcp = &s_dhcp;
  uint8_t mac_address[6];
  macAddress(mac_address);
  stm32_eth_init(mac_address, NULL, NULL, NULL);

  // Now try to get our config info from a DHCP server
  int ret = _dhcp->beginWithDHCP(mac_address, timeout, responseTimeout);
  if(ret == 1)
  {
    _dnsServerAddress = _dhcp->getDnsServerIp();
  }

  return ret;
}

void EthernetClass::begin(IPAddress local_ip)
{
  IPAddress subnet(255, 255, 255, 0);
  begin(local_ip, subnet);
}

void EthernetClass::begin(IPAddress local_ip, IPAddress subnet)
{
  // Assume the gateway will be the machine on the same network as the local IP
  // but with last octet being '1'
  IPAddress gateway = local_ip;
  gateway[3] = 1;
  begin(local_ip, subnet, gateway);
}

void EthernetClass::begin(IPAddress local_ip, IPAddress subnet, IPAddress gateway)
{
  // Assume the DNS server will be the machine on the same network as the local IP
  // but with last octet being '1'
  IPAddress dns_server = local_ip;
  dns_server[3] = 1;
  begin(local_ip, subnet, gateway, dns_server);
}

void EthernetClass::begin(IPAddress local_ip, IPAddress subnet, IPAddress gateway, IPAddress dns_server)
{
  uint8_t mac_address[6];
  macAddress(mac_address);
  stm32_eth_init(mac_address, local_ip.raw_address(), gateway.raw_address(), subnet.raw_address());
  /* If there is a local DHCP informs it of our manual IP configuration to
  prevent IP conflict */
  stm32_DHCP_manual_config();
  _dnsServerAddress = dns_server;
}

int EthernetClass::maintain(){
  int rc = DHCP_CHECK_NONE;

  if(_dhcp != NULL){
    //we have a pointer to dhcp, use it
    rc = _dhcp->checkLease();
    switch ( rc ){
      case DHCP_CHECK_NONE:
        //nothing done
        break;
      case DHCP_CHECK_RENEW_OK:
      case DHCP_CHECK_REBIND_OK:
        _dnsServerAddress = _dhcp->getDnsServerIp();
        break;
      default:
        //this is actually a error, it will retry though
        break;
    }
  }
  return rc;
}

/*
 * This function updates the LwIP stack and can be called to be sure to update
 * the stack (e.g. in case of a long loop).
 */
void EthernetClass::schedule(void)
{
  stm32_eth_scheduler();
}

void EthernetClass::macAddress(uint8_t *mac)
{
  // Read unique id
  #if defined (STM32F2)
    uint32_t baseUID = *(uint32_t *)0x1FFF7A10;
  #elif defined (STM32F4)
    uint32_t baseUID = *(uint32_t *)0x1FFF7A10;
  #elif defined (STM32F7)
    uint32_t baseUID = *(uint32_t *)0x1FF0F420;
  #else
    #error MAC address can not be derived from target unique Id
  #endif
    
  mac[0] = 0x00;
  mac[1] = 0x80;
  mac[2] = 0xE1;
  mac[3] = (baseUID & 0x00FF0000) >> 16;
  mac[4] = (baseUID & 0x0000FF00) >> 8;
  mac[5] = (baseUID & 0x000000FF);
}

IPAddress EthernetClass::localIP()
{
  return IPAddress(stm32_eth_get_ipaddr());
}

IPAddress EthernetClass::subnetMask()
{
  return IPAddress(stm32_eth_get_netmaskaddr());
}

IPAddress EthernetClass::gatewayIP()
{
  return IPAddress(stm32_eth_get_gwaddr());
}

IPAddress EthernetClass::dnsServerIP()
{
  return _dnsServerAddress;
}

EthernetClass Ethernet;
