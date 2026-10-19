/**
 * @file    EthernetServer.h
 * @brief   TCP server class of the Ethernet library.
 *
 * Declares EthernetServer, which listens on a TCP port and accepts up to
 * MAX_CLIENT clients.
 */
#ifndef ethernetserver_h
#define ethernetserver_h

#include "Server.h"

class EthernetClient;

class EthernetServer :
  public Server {
  private:
    uint16_t _port;
    struct tcp_struct _tcp_server;
    struct tcp_struct *_tcp_client[MAX_CLIENT];

    /**
     * @brief Releases the slots of clients whose connection is closing.
     *
     * Does not accept new connections. Incoming connections are accepted by
     * the TCP accept callback.
     */
    void accept(void);
  public:
    /**
     * @brief Creates a server for a TCP port.
     *
     * The server does not listen until begin() is called.
     *
     * @param port TCP port to listen on. Defaults to 80.
     */
    EthernetServer(uint16_t port = 80);
    /**
     * @brief Returns a client that has data ready to read.
     *
     * Releases closed client slots first. The returned client shares the
     * connection structure of the server's slot.
     *
     * @return An accepted client with data available, or a client with no
     * connection if none has data.
     */
    EthernetClient available();
    /**
     * @brief Starts listening on the current port.
     *
     * Does nothing if the server is already listening or if the TCP control
     * block cannot be created or bound. Failures are not reported.
     */
    virtual void begin();
    /**
     * @brief Sets a new port and starts listening on it.
     *
     * @param port TCP port to listen on.
     */
    virtual void begin(uint16_t port);
    /**
     * @brief Stops the server.
     *
     * Closes and frees all accepted clients, then closes the listening
     * connection.
     */
    void end(void);
    /**
     * @brief Writes one byte to every accepted client.
     *
     * @param b Byte to send.
     * @return The total number of bytes written to all clients.
     */
    virtual size_t write(uint8_t b);
    /**
     * @brief Writes a buffer to every accepted client.
     *
     * The same data is sent to each client. Clients that are not accepted
     * are skipped.
     *
     * @param buffer Data to send.
     * @param size Number of bytes in buffer.
     * @return The sum of the bytes written to each client. This can be larger
     * than size when several clients are connected.
     */
    virtual size_t write(const uint8_t *buffer, size_t size);
    /**
     * @brief Tests whether the server is listening.
     *
     * @return true if the server has a listening connection, false otherwise.
     */
    virtual operator bool();
    using Print::write;
};

#endif
