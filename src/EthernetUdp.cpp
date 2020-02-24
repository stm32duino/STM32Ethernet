/*
 *  Udp.cpp: Library to send/receive UDP packets with the Arduino ethernet shield.
 *  This version only offers minimal wrapping of socket.c/socket.h
 *  Drop Udp.h/.cpp into the Ethernet library directory at hardware/libraries/Ethernet/
 *
 * MIT License:
 * Copyright (c) 2008 Bjoern Hartmann
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * bjoern@cs.stanford.edu 12/30/2008
 */
#include "STM32Ethernet.h"
#include "Udp.h"
#include "Dns.h"

#include "lwip/igmp.h"
#include "lwip/ip_addr.h"

/* Constructor */
EthernetUDP::EthernetUDP() {}

/* Start EthernetUDP socket, listening at local port PORT */
uint8_t EthernetUDP::begin(uint16_t port)
{
  return begin(Ethernet.localIP(), port);
}

/* Start EthernetUDP socket, listening at local IP ip and port PORT */
uint8_t EthernetUDP::begin(IPAddress ip, uint16_t port, bool multicast)
{
  // Can create a single udp connection per socket
  if (_udp.pcb != NULL) {
    return 0;
  }

  _udp.pcb = udp_new();

  if (_udp.pcb == NULL) {
    return 0;
  }

  ip_addr_t ipaddr;
  err_t err;
  u8_to_ip_addr(rawIPAddress(ip), &ipaddr);
  if (multicast) {
    err = udp_bind(_udp.pcb, IP_ADDR_ANY, port);
  } else {
    err = udp_bind(_udp.pcb, &ipaddr, port);
  }

  if (ERR_OK != err) {
    stop();
    return 0;
  }

#if LWIP_IGMP
  if ((multicast) && (ERR_OK != igmp_joingroup(IP_ADDR_ANY, &ipaddr))) {
    return 0;
  }
#endif
  udp_recv(_udp.pcb, &udp_receive_callback, &_udp);

  _port = port;
  _remaining = 0;

  stm32_eth_scheduler();

  return 1;
}

/* return number of bytes available in the current packet,
   will return zero if parsePacket hasn't been called yet */
int EthernetUDP::available()
{
  return _remaining;
}

/* Release any resources being used by this EthernetUDP instance */
void EthernetUDP::stop()
{
  if (_udp.pcb != NULL) {
    udp_disconnect(_udp.pcb);
    udp_remove(_udp.pcb);
    _udp.pcb = NULL;
  }

  stm32_eth_scheduler();
}

int EthernetUDP::beginPacket(const char *host, uint16_t port)
{
  // Look up the host first
  int ret = 0;
  DNSClient dns;
  IPAddress remote_addr;

  dns.begin(Ethernet.dnsServerIP());
  ret = dns.getHostByName(host, remote_addr);
  if (ret == 1) {
    return beginPacket(remote_addr, port);
  } else {
    return ret;
  }
}

int EthernetUDP::beginPacket(IPAddress ip, uint16_t port)
{
  if (_udp.pcb == NULL) {
    return 0;
  }

  _sendtoIP = ip;
  _sendtoPort = port;

  udp_recv(_udp.pcb, &udp_receive_callback, &_udp);
  stm32_eth_scheduler();

  return 1;
}

int EthernetUDP::endPacket()
{
  if ((_udp.pcb == NULL) || (_data == NULL)) {
    return 0;
  }

  ip_addr_t ipaddr;
  if (ERR_OK != udp_sendto(_udp.pcb, _data, u8_to_ip_addr(rawIPAddress(_sendtoIP), &ipaddr), _sendtoPort)) {
    _data = stm32_free_data(_data);
    return 0;
  }

  _data = stm32_free_data(_data);
  stm32_eth_scheduler();

  return 1;
}

size_t EthernetUDP::write(uint8_t byte)
{
  return write(&byte, 1);
}

size_t EthernetUDP::write(const uint8_t *buffer, size_t size)
{
  _data = stm32_new_data(_data, buffer, size);
  if (_data == NULL) {
    return 0;
  }

  return size;
}

int EthernetUDP::parsePacket()
{
  // discard any remaining bytes in the last packet
  // while (_remaining) {
  //   // could this fail (loop endlessly) if _remaining > 0 and recv in read fails?
  //   // should only occur if recv fails after telling us the data is there, lets
  //   // hope the w5100 always behaves :)
  //   read();
  // }

  stm32_eth_scheduler();

  if (_udp.data.available > 0) {
    _remoteIP = IPAddress(ip_addr_to_u32(&(_udp.ip)));
    _remotePort = _udp.port;
    _remaining = _udp.data.available;

    return _remaining;
  }
  // There aren't any packets available
  return 0;
}

int EthernetUDP::read()
{
  uint8_t byte;

  if (_udp.data.p == NULL) {
    return -1;
  }

  if (_remaining > 0) {
    if (read(&byte, 1) > 0) {
      return byte;
    }
  }

  // If we get here, there's no data available
  return -1;
}

int EthernetUDP::read(unsigned char *buffer, size_t len)
{
  if (_udp.data.p == NULL) {
    return -1;
  }

  if (_remaining > 0) {
    int got;

    if (_remaining <= len) {
      // data should fit in the buffer
      got = (int)stm32_get_data(&(_udp.data), (uint8_t *)buffer, _remaining);
    } else {
      // too much data for the buffer,
      // grab as much as will fit
      got = (int)stm32_get_data(&(_udp.data), (uint8_t *)buffer, len);
    }

    if (got > 0) {
      _remaining -= got;
      return got;
    }

  }

  // If we get here, there's no data available or recv failed
  return -1;

}

int EthernetUDP::peek()
{
  uint8_t b;
  // Unlike recv, peek doesn't check to see if there's any data available, so we must.
  // If the user hasn't called parsePacket yet then return nothing otherwise they
  // may get the UDP header
  if (!_remaining) {
    return -1;
  }
  b = pbuf_get_at(_udp.data.p, 0);
  return b;
}

void EthernetUDP::flush()
{
  // TODO: we should wait for TX buffer to be emptied
}

/* Start EthernetUDP socket, listening at local port PORT */
uint8_t EthernetUDP::beginMulticast(IPAddress ip, uint16_t port)
{
  return begin(ip, port, true);
}

#if LWIP_UDP
void EthernetUDP::onDataArrival(std::function<void()> onDataArrival_fn)
{
  _udp.onDataArrival = onDataArrival_fn;
}
#endif
