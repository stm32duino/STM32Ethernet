/**
 * @file    EthernetUdp.h
 * @brief   UDP class of the Ethernet library.
 *
 * Declares EthernetUDP, which implements the Arduino UDP interface (send and
 * receive datagrams, optional multicast) on top of LwIP.
 */
/*
 *  Udp.cpp: Library to send/receive UDP packets with the Arduino ethernet shield.
 *  This version only offers minimal wrapping of socket.c/socket.h
 *  Drop Udp.h/.cpp into the Ethernet library directory at hardware/libraries/Ethernet/
 *
 * NOTE: UDP is fast, but has some important limitations (thanks to Warren Gray for mentioning these)
 * 1) UDP does not guarantee the order in which assembled UDP packets are received. This
 * might not happen often in practice, but in larger network topologies, a UDP
 * packet can be received out of sequence.
 * 2) UDP does not guard against lost packets - so packets *can* disappear without the sender being
 * aware of it. Again, this may not be a concern in practice on small local networks.
 * For more information, see http://www.cafeaulait.org/course/week12/35.html
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

#ifndef ethernetudp_h
#define ethernetudp_h

#include <Udp.h>
#include <functional>

#include "utility/stm32_eth.h"

#define UDP_TX_PACKET_MAX_SIZE 24

class EthernetUDP : public UDP {
  private:
    uint16_t _port; // local port to listen on
    IPAddress _remoteIP; // remote IP address for the incoming packet whilst it's being processed
    uint16_t _remotePort; // remote port for the incoming packet whilst it's being processed
    IPAddress _sendtoIP;  // the remote IP address set by beginPacket
    uint16_t _sendtoPort; // the remote port set by beginPacket

    struct pbuf *_data;     //pbuf for data to send
    struct udp_struct _udp; //udp settings

  protected:
    uint16_t _remaining; // remaining bytes of incoming packet yet to be processed

  public:
    /**
     * @brief Constructs an EthernetUDP object without opening a socket.
     *
     * Call begin() or beginMulticast() to open a socket.
     */
    EthernetUDP();
    /**
     * @brief Opens a unicast UDP socket on a local port.
     *
     * The socket is bound to the address returned by Ethernet.localIP().
     *
     * @param port Local port to listen on.
     * @return 1 on success, 0 on failure; see the IPAddress overload.
     */
    virtual uint8_t begin(uint16_t);
    /**
     * @brief Opens a UDP socket bound to a local address and port.
     *
     * Fails if this object already has an open socket. When multicast is
     * true, the socket binds to any address and joins the multicast group
     * ip through IGMP (when LWIP_IGMP is enabled). Otherwise it binds to ip.
     * If the bind fails, the socket is closed. If the IGMP join fails, the
     * socket stays open; call stop() to close it.
     *
     * @param ip Local address to bind to, or the multicast group to join.
     * @param port Local port to listen on.
     * @param multicast true to join the multicast group ip, false for a
     *                  unicast socket bound to ip.
     * @return 1 on success. 0 if a socket is already open, if the UDP PCB
     *         cannot be created or bound, or if the IGMP join fails.
     */
    virtual uint8_t begin(IPAddress, uint16_t, bool multicast = false);
    /**
     * @brief Opens a UDP socket that joins a multicast group.
     *
     * Equivalent to begin(ip, port, true).
     *
     * @param ip Multicast group address to join.
     * @param port Local port to listen on.
     * @return 1 on success, 0 on failure (same as begin()).
     */
    virtual uint8_t beginMulticast(IPAddress, uint16_t);
    /**
     * @brief Closes the UDP socket.
     *
     * Does nothing if no socket is open.
     */
    virtual void stop();

    // Sending UDP packets

    /**
     * @brief Sets the destination for the next outgoing packet.
     *
     * The address and port are stored as given and not validated. Call
     * write() and endPacket() afterwards.
     *
     * @param ip Destination IP address.
     * @param port Destination port.
     * @return 1 on success, 0 if no socket is open.
     */
    virtual int beginPacket(IPAddress ip, uint16_t port);
    /**
     * @brief Sets the destination for the next outgoing packet by host name.
     *
     * Resolves host with a DNS query to the server returned by
     * Ethernet.dnsServerIP(), then calls beginPacket(IPAddress, uint16_t).
     *
     * @param host Host name, or a numeric address string.
     * @param port Destination port.
     * @return 1 on success. If the lookup does not succeed, the negative DNS
     *         error code from DNSClient::getHostByName(). Otherwise the
     *         result of beginPacket(IPAddress, uint16_t).
     */
    virtual int beginPacket(const char *host, uint16_t port);
    /**
     * @brief Sends the data written since beginPacket() as one datagram.
     *
     * The buffered data is released whether or not the send succeeds.
     *
     * @return 1 if the packet was sent, 0 if no socket is open, no data was
     *         written, or the send failed.
     */
    virtual int endPacket();
    /**
     * @brief Appends one byte to the outgoing packet.
     *
     * @param byte Byte to append.
     * @return 1 if the byte was stored, 0 on allocation failure.
     */
    virtual size_t write(uint8_t);
    /**
     * @brief Appends a block of bytes to the outgoing packet.
     *
     * Data accumulates until endPacket() sends it. On failure, the data
     * already buffered for the packet is kept and the new bytes are not added.
     *
     * @param buffer Bytes to append.
     * @param size Number of bytes in buffer.
     * @return size on success, 0 on failure.
     */
    virtual size_t write(const uint8_t *buffer, size_t size);

    using Print::write;

    /**
     * @brief Advances to the next received packet.
     *
     * Runs the Ethernet scheduler. If a datagram is queued, records its
     * sender for remoteIP() and remotePort() and its unread size for
     * available() and read().
     *
     * @return Size of the queued packet in bytes, or 0 if none is queued.
     */
    virtual int parsePacket();
    /**
     * @brief Returns the number of unread bytes in the current packet.
     *
     * Returns 0 until parsePacket() has been called.
     *
     * @return Number of bytes remaining in the current packet.
     */
    virtual int available();
    /**
     * @brief Reads one byte from the current packet.
     *
     * @return The byte value (0-255), or -1 if no data is available.
     */
    virtual int read();
    /**
     * @brief Reads up to len bytes from the current packet into buffer.
     *
     * Reads at most the number of unread bytes in the packet.
     *
     * @param buffer Destination buffer; must hold at least len bytes.
     * @param len Maximum number of bytes to read.
     * @return Number of bytes read, or -1 if no data is available.
     */
    virtual int read(unsigned char *buffer, size_t len);
    /**
     * @brief Reads up to len characters from the current packet into buffer.
     *
     * Forwards to read(unsigned char *, size_t).
     *
     * @param buffer Destination buffer; must hold at least len bytes.
     * @param len Maximum number of characters to read.
     * @return Number of characters read, or -1 if no data is available.
     */
    virtual int read(char *buffer, size_t len)
    {
      return read((unsigned char *)buffer, len);
    };
    /**
     * @brief Returns the next byte of the current packet without consuming it.
     *
     * @return The byte value (0-255), or -1 if no unread data is available.
     */
    virtual int peek();
    /**
     * @brief Intended to wait for the transmit buffer to empty.
     *
     * Currently a no-op. Waiting for the transmit buffer is a TODO.
     */
    virtual void flush();

    /**
     * @brief Returns the IP address of the sender of the current packet.
     *
     * The value is set by parsePacket().
     *
     * @return Remote IP address.
     */
    virtual IPAddress remoteIP()
    {
      return _remoteIP;
    };
    /**
     * @brief Returns the port of the sender of the current packet.
     *
     * The value is set by parsePacket().
     *
     * @return Remote port number.
     */
    virtual uint16_t remotePort()
    {
      return _remotePort;
    };
    /**
     * @brief Registers a callback invoked when a UDP datagram arrives.
     *
     * Replaces any previously registered callback.
     *
     * @param onDataArrival_fn Function to call when a datagram arrives.
     */
    virtual void onDataArrival(std::function<void()> onDataArrival_fn);
};

#endif
