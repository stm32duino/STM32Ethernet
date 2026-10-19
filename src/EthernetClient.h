/**
 * @file    EthernetClient.h
 * @brief   TCP client class of the Ethernet library.
 *
 * Declares EthernetClient, which implements the Arduino Client interface on top
 * of an LwIP TCP connection.
 */
#ifndef ethernetclient_h
#define ethernetclient_h
#include "Arduino.h"
#include "Print.h"
#include "Client.h"
#include "IPAddress.h"
#include "utility/stm32_eth.h"

class EthernetClient : public Client {

  public:
    /**
     * @brief Default constructor. Creates a client with no connection.
     */
    EthernetClient();

    /**
     * @brief Deprecated constructor kept for W5100 sketch compatibility.
     *
     * The socket argument is ignored and the client starts with no connection.
     *
     * @param sock Socket number, ignored.
     */
    EthernetClient(uint8_t sock);

    /**
     * @brief Wraps an existing TCP connection.
     *
     * The structure is used directly and is not copied. EthernetServer uses
     * this constructor to return accepted clients.
     *
     * @param tcpClient Pointer to the TCP connection structure to wrap.
     */
    EthernetClient(struct tcp_struct *tcpClient);

    /**
     * @brief Returns the state of the TCP connection.
     *
     * @return A tcp_client_states value, or TCP_NONE if there is no connection
     * structure.
     */
    uint8_t status();
    /**
     * @brief Connects to a remote host by IP address.
     *
     * Blocks until the connection is established, fails, or the timeout set
     * with setConnectionTimeout() expires.
     *
     * @param ip Remote IP address.
     * @param port Remote TCP port.
     * @return 1 if the connection was established, 0 otherwise.
     */
    virtual int connect(IPAddress ip, uint16_t port);
    /**
     * @brief Connects to a remote host by host name.
     *
     * Resolves the name with the DNS server configured in Ethernet, then
     * connects to the resulting address.
     *
     * @param host Host name to resolve.
     * @param port Remote TCP port.
     * @return 1 if the connection was established, 0 otherwise, including when
     * the name cannot be resolved.
     */
    virtual int connect(const char *host, uint16_t port);
    /**
     * @brief Writes one byte to the connection.
     *
     * @param b Byte to send.
     * @return 1 if the byte was written, 0 on failure.
     */
    virtual size_t write(uint8_t b);
    /**
     * @brief Writes a buffer to the connection.
     *
     * Sends the data in chunks limited by the free space in the TCP send
     * buffer, and forces each chunk out. Bytes queued before a TCP error are
     * not reported.
     *
     * @param buf Data to send.
     * @param size Number of bytes in buf.
     * @return size if all bytes were queued, 0 on failure. Failure includes a
     * client that is not connected or accepted, a NULL buf, size 0, and a TCP
     * error.
     */
    virtual size_t write(const uint8_t *buf, size_t size);
    /**
     * @brief Returns the number of bytes ready to be read.
     *
     * Runs the network scheduler before checking the receive buffer.
     *
     * @return Number of bytes available, or 0 if there is no connection.
     */
    virtual int available();
    /**
     * @brief Reads one byte from the receive buffer.
     *
     * @return The byte read (0 to 255), or -1 if no data is buffered.
     */
    virtual int read();
    /**
     * @brief Reads up to size bytes from the receive buffer.
     *
     * @param buf Destination buffer.
     * @param size Maximum number of bytes to read.
     * @return Number of bytes copied into buf, or -1 if no data is buffered.
     */
    virtual int read(uint8_t *buf, size_t size);
    /**
     * @brief Returns the next byte without removing it from the buffer.
     *
     * Runs the network scheduler through available().
     *
     * @return The next byte (0 to 255), or -1 if no data is available.
     */
    virtual int peek();
    /**
     * @brief Sends any data still queued for transmission.
     *
     * Does not wait for the remote host to acknowledge the data.
     */
    virtual void flush();
    /**
     * @brief Closes the connection and releases the client structure.
     *
     * The connection is closed unless it is already in TCP_CLOSING state. The
     * structure is freed and the client is left with no connection.
     */
    virtual void stop();
    /**
     * @brief Checks whether the client is connected.
     *
     * A client whose connection is closing still counts as connected while
     * data remains to be read.
     *
     * @return 1 if connected, 0 otherwise.
     */
    virtual uint8_t connected();
    /**
     * @brief Tests whether the client holds a usable connection.
     *
     * Lets the client returned by EthernetServer::available() be used as the
     * condition of an if-statement.
     *
     * @return true if a connection structure exists and is not in TCP_CLOSING
     * state, false otherwise.
     */
    virtual operator bool();
    /**
     * @brief Compares the client's boolean value with a value.
     *
     * @param value Value to compare with operator bool().
     * @return true if the client's boolean value equals value.
     */
    virtual bool operator==(const bool value)
    {
      return bool() == value;
    }
    /**
     * @brief Compares the client's boolean value with a value.
     *
     * @param value Value to compare with operator bool().
     * @return true if the client's boolean value differs from value.
     */
    virtual bool operator!=(const bool value)
    {
      return bool() != value;
    }
    /**
     * @brief Tests whether two clients refer to the same connection.
     *
     * Two clients with no connection structure compare equal, and the function
     * then reads a NULL pointer. Only compare clients that have a connection.
     *
     * @param rhs Client to compare with.
     * @return true if both clients use the same connection structure and the
     * same TCP control block, false otherwise.
     */
    virtual bool operator==(const EthernetClient &);
    /**
     * @brief Tests whether two clients refer to different connections.
     *
     * @param rhs Client to compare with.
     * @return The negation of operator==(rhs).
     */
    virtual bool operator!=(const EthernetClient &rhs)
    {
      return !this->operator==(rhs);
    };
    /**
     * @brief Returns the socket number, for W5100 compatibility.
     *
     * This function is specific to the W5100 architecture and is not defined by
     * Arduino. It is kept for compatibility and always returns 0 because this
     * implementation has no socket numbers.
     *
     * @return Always 0.
     */
    uint8_t getSocketNumber();
    /**
     * @brief Returns the local TCP port of the connection.
     *
     * Requires an active connection. No NULL check is performed.
     *
     * @return Local port number.
     */
    virtual uint16_t localPort()
    {
      return (_tcp_client->pcb->local_port);
    };
    /**
     * @brief Returns the remote IP address of the connection.
     *
     * Requires an active connection. No NULL check is performed.
     *
     * @return Remote IP address.
     */
    virtual IPAddress remoteIP()
    {
      return (IPAddress(_tcp_client->pcb->remote_ip.addr));
    };
    /**
     * @brief Returns the remote TCP port of the connection.
     *
     * Requires an active connection. No NULL check is performed.
     *
     * @return Remote port number.
     */
    virtual uint16_t remotePort()
    {
      return (_tcp_client->pcb->remote_port);
    };
    /**
     * @brief Sets how long connect() waits for the connection (ms).
     *
     * @param timeout Timeout in milliseconds.
     */
    void setConnectionTimeout(uint16_t timeout)
    {
      _connectionTimeout = timeout;
    }

    friend class EthernetServer;

    using Print::write;

  private:
    struct tcp_struct *_tcp_client;
    uint16_t _connectionTimeout = 10000;
};

#endif
