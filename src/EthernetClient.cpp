extern "C" {
#include "string.h"
}

#include "Arduino.h"

#include "STM32Ethernet.h"
#include "EthernetClient.h"
#include "EthernetServer.h"
#include "Dns.h"

EthernetClient::EthernetClient()
  : _tcp_client(NULL)
{
}

/* Deprecated constructor. Keeps compatibility with W5100 architecture
sketches but sock is ignored. */
EthernetClient::EthernetClient(uint8_t sock)
  : _tcp_client(NULL)
{
  UNUSED(sock);
}

EthernetClient::EthernetClient(struct tcp_struct *tcpClient)
{
  _tcp_client = tcpClient;
}

int EthernetClient::connect(const char *host, uint16_t port)
{
  // Look up the host first
  int ret = 0;
  DNSClient dns;
  IPAddress remote_addr;

  dns.begin(Ethernet.dnsServerIP());
  ret = dns.getHostByName(host, remote_addr);
  if (ret == 1) {
    return connect(remote_addr, port);
  } else {
    return ret;
  }
}

int EthernetClient::connect(IPAddress ip, uint16_t port)
{
  if (_tcp_client == NULL) {
    /* Allocates memory for client */
    _tcp_client = (struct tcp_struct *)mem_malloc(sizeof(struct tcp_struct));

    if (_tcp_client == NULL) {
      return 0;
    }
  }

  /* Creates a new TCP protocol control block */
  _tcp_client->pcb = tcp_new();

  if (_tcp_client->pcb == NULL) {
    return 0;
  }

  _tcp_client->data.p = NULL;
  _tcp_client->data.available = 0;
  _tcp_client->state = TCP_NONE;

  uint32_t startTime = millis();
  ip_addr_t ipaddr;
  tcp_arg(_tcp_client->pcb, _tcp_client);
  if (ERR_OK != tcp_connect(_tcp_client->pcb, u8_to_ip_addr(rawIPAddress(ip), &ipaddr), port, &tcp_connected_callback)) {
    stop();
    return 0;
  }

  startTime = millis();
  while (_tcp_client->state == TCP_NONE) {
    stm32_eth_scheduler();
    if ((_tcp_client->state == TCP_CLOSING) || ((millis() - startTime) >= _timeout)) {
      stop();
      return 0;
    }
  }

  return 1;
}

size_t EthernetClient::write(uint8_t b)
{
  return write(&b, 1);
}

size_t EthernetClient::write(const uint8_t *buf, size_t size)
{
  if ((_tcp_client == NULL) || (_tcp_client->pcb == NULL) ||
      (buf == NULL) || (size == 0)) {
    return 0;
  }

  /* If client not connected or accepted, it can't write because connection is
  not ready */
  if ((_tcp_client->state != TCP_ACCEPTED) &&
      (_tcp_client->state != TCP_CONNECTED)) {
    return 0;
  }

  size_t max_send_size, bytes_to_send;
  size_t bytes_sent = 0;
  size_t bytes_left = size;
  err_t res;

  do {
    max_send_size = tcp_sndbuf(_tcp_client->pcb);
    bytes_to_send = bytes_left > max_send_size ? max_send_size : bytes_left;

    if (bytes_to_send > 0) {
      res = tcp_write(_tcp_client->pcb, &buf[bytes_sent], bytes_to_send,  TCP_WRITE_FLAG_COPY);

      if (res == ERR_OK) {
        bytes_sent += bytes_to_send;
        bytes_left = size - bytes_sent;
      } else if (res != ERR_MEM) {
        // other error, cannot continue
        return 0;
      }
    }

    //Force to send data right now!
    if (ERR_OK != tcp_output(_tcp_client->pcb)) {
      return 0;
    }
    stm32_eth_scheduler();

  } while (bytes_sent != size);

  return size;
}

int EthernetClient::available()
{
  stm32_eth_scheduler();
  if (_tcp_client != NULL) {
    return _tcp_client->data.available;
  }
  return 0;
}

int EthernetClient::read()
{
  uint8_t b;
  if ((_tcp_client != NULL) && (_tcp_client->data.p != NULL)) {
    stm32_get_data(&(_tcp_client->data), &b, 1);
    return b;
  }
  // No data available
  return -1;
}

int EthernetClient::read(uint8_t *buf, size_t size)
{
  if ((_tcp_client != NULL) && (_tcp_client->data.p != NULL)) {
    return stm32_get_data(&(_tcp_client->data), buf, size);
  }
  return -1;
}

int EthernetClient::peek()
{
  uint8_t b;
  // Unlike recv, peek doesn't check to see if there's any data available, so we must
  if (!available()) {
    return -1;
  }
  b = pbuf_get_at(_tcp_client->data.p, 0);
  return b;
}

void EthernetClient::flush()
{
  if ((_tcp_client == NULL) || (_tcp_client->pcb == NULL)) {
    return;
  }
  tcp_output(_tcp_client->pcb);
  stm32_eth_scheduler();
}

void EthernetClient::stop()
{
  if (_tcp_client == NULL) {
    return;
  }

  // close tcp connection if not closed yet
  if (status() != TCP_CLOSING) {
    tcp_connection_close(_tcp_client->pcb, _tcp_client);
  }
}

uint8_t EthernetClient::connected()
{
  uint8_t s = status();
  return ((available() && (s == TCP_CLOSING)) ||
          (s == TCP_CONNECTED) || (s == TCP_ACCEPTED));
}

uint8_t EthernetClient::status()
{
  if (_tcp_client == NULL) {
    return TCP_NONE;
  }
  return _tcp_client->state;
}

// the next function allows us to use the client returned by
// EthernetServer::available() as the condition in an if-statement.

EthernetClient::operator bool()
{
  return (_tcp_client && (_tcp_client->state != TCP_CLOSING));
}

bool EthernetClient::operator==(const EthernetClient &rhs)
{
  return _tcp_client == rhs._tcp_client && _tcp_client->pcb == rhs._tcp_client->pcb;
}

/* This function is not a function defined by Arduino. This is a function
specific to the W5100 architecture. To keep the compatibility we leave it and
returns always 0. */
uint8_t EthernetClient::getSocketNumber()
{
  return 0;
}
