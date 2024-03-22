/**
* @file TCPServer.h
*/
#pragma once
#include <netinet/in.h>
#include <string>
#include <thread>

// Declaration of the TCPServer class.
class TCPServer {
public:
    // Constructor: Initializes a new instance of the TCPServer with a specific port.
    // port: The TCP port number on which the server will listen for incoming connections.
    TCPServer(uint16_t port);

    // Destructor: Cleans up resources when a TCPServer object is destroyed. This includes stopping the server if it is running.
    ~TCPServer();

    // Starts the server: Sets up the TCP socket, binds it to the specified port, listens for incoming connections,
    // and launches a thread to handle these connections asynchronously.
    void start();

    // Stops the server: Stops the server from accepting new connections, closes any active connection, and cleans up resources.
    void stop();

    // Sends data to the connected client.
    // data: A vector of unsigned chars representing the data to be sent to the client.
    void send(const std::vector<unsigned char>& data);

private:
    // The port number on which the server listens for incoming connections.
    uint16_t port;

    // File descriptor for the server socket. Used to manage the listening socket.
    int serverFd;

    // File descriptor for the client socket. Represents a socket for communication with the connected client.
    int clientFd;

    // A thread object for accepting connections. This thread runs the acceptConnections method,
    // allowing the server to handle incoming connections without blocking the main thread.
    std::thread acceptThread;

    // A boolean flag indicating whether the server is currently running. This is used to control the loop
    // in the acceptConnections method and to manage the server's running state.
    bool running;

    // A private method that continuously accepts incoming connections when the server is running.
    // This method is intended to be executed in a separate thread.
    void acceptConnections();
};