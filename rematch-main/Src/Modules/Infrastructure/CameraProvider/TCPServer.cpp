/**
* @file TCPServer.cpp
*/
#include "TCPServer.h"
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>

// Constructor for the TCPServer class. Initializes the server with the specified port number.
// The server and client file descriptors are initialized to -1 to indicate they are not in use.
// The server's running state is initialized to false.
TCPServer::TCPServer(uint16_t port) : port(port), serverFd(-1), clientFd(-1), running(false) {}

// Destructor for the TCPServer class. It ensures that the server is stopped and resources are released properly.
TCPServer::~TCPServer() {
    stop();
}

// Starts the server: creates a socket, binds it to the specified port on any available interface,
// listens for incoming connections, and starts a thread to accept connections.
void TCPServer::start() {
    // Create a socket for the server using IPv4 and TCP.
    serverFd = socket(AF_INET, SOCK_STREAM, 0);

    // Define the server address and port.
    sockaddr_in address{};
    address.sin_family = AF_INET; // Address family: IPv4
    address.sin_addr.s_addr = INADDR_ANY; // Listen on any network interface.
    address.sin_port = htons(port); // Convert port number to network byte order.

    // Bind the server socket to the specified address and port.
    bind(serverFd, reinterpret_cast<sockaddr*>(&address), sizeof(address));

    // Start listening for incoming connections. The backlog parameter is set to 1.
    listen(serverFd, 1);

    // Mark the server as running.
    running = true;

    // Start a new thread to handle incoming connections.
    acceptThread = std::thread(&TCPServer::acceptConnections, this);
}

// Stops the server: stops accepting connections, joins the accept thread,
// closes any open client and server sockets, and marks the server as not running.
void TCPServer::stop() {
    // Mark the server as not running.
    running = false;

    // If the accept thread is running, wait for it to finish.
    if (acceptThread.joinable()) {
        acceptThread.join();
    }

    // If a client is connected, close the client socket.
    if (clientFd != -1) {
        close(clientFd);
        clientFd = -1;
    }

    // Close the server socket if it's open.
    if (serverFd != -1) {
        close(serverFd);
        serverFd = -1;
    }
}

// Sends data to the connected client.
void TCPServer::send(const std::vector<unsigned char>& data) {
    // If a client is connected, send the data.
    if (clientFd != -1) {
        ::send(clientFd, data.data(), data.size(), 0);
    }
}

// Accepts incoming client connections. This method runs in a separate thread and continuously
// accepts new connections until the server is stopped. It handles one connection at a time.
void TCPServer::acceptConnections() {
    while (running) {
        // Accept a new connection. This call is blocking and waits for a new client to connect.
        clientFd = accept(serverFd, nullptr, nullptr);

        // If accepting a client connection fails, stop the server.
        if (clientFd < 0) {
            running = false;
        }
    }
}