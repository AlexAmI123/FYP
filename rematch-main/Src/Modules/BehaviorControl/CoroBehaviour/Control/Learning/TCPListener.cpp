/**
* @file TCPListener.cpp
*/
#include "TCPListener.h"
#include <netinet/in.h>
#include <cstring>
#include <unistd.h>
#include <iostream>

// Constructor for the TCPListener class. It initializes the listenThread pointer to nullptr.
TCPListener::TCPListener() : listenThread(nullptr) {}

// Destructor for the TCPListener class. Ensures the listening thread is joined properly and deallocates memory.
TCPListener::~TCPListener() {
    // Check if listenThread is not null and joinable, then join it.
    if (listenThread && listenThread->joinable()) {
        listenThread->join();
    }
    // Free the memory allocated for the listenThread.
    delete listenThread;
}

// Method to start listening on a specified port. It spawns a new thread to handle incoming connections.
void TCPListener::startListening(uint16_t port) {
    // Allocate a new thread for listening to incoming connections and pass the listenerThread static method as the thread function.
    listenThread = new std::thread(listenerThread, this, port);
}

// Static method to handle incoming connections. This method is intended to run on a separate thread.
void TCPListener::listenerThread(TCPListener* listener, uint16_t port) {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int opt = 1; // Used to set socket options.
    int addrlen = sizeof(address); // Length of the address data structure.
    char buffer[1024] = {0}; // Buffer for reading incoming data.

    // Create a socket for IPv4 with TCP.
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options to allow reuse of local addresses.
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Configure the address structure for the server socket.
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces.
    address.sin_port = htons(port); // Convert the port number to network byte order.

    // Bind the server socket to the specified port.
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections, with a maximum backlog of 3 pending connections.
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // Accept a connection. The client_socket represents the connection to the client.
    if ((client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    // Continuously read data from the client socket until there's no more data.
    while (read(client_socket, buffer, 1024) > 0) {
        // Enqueue the read command into the listener's command queue.
        listener->enqueueCommand(std::string(buffer));
        // Clear the buffer for the next read operation.
        memset(buffer, 0, 1024);
    }

    // Close the server socket once done.
    close(server_fd);
}

// Method to add a command to the internal command queue, using mutex for thread safety.
void TCPListener::enqueueCommand(const std::string& command) {
    std::lock_guard<std::mutex> lock(commandQueueMutex); // Ensure thread-safe access to the command queue.
    commandQueue.push(command); // Add the command to the queue.
}

// Method to remove and return the next command from the queue, if available.
bool TCPListener::dequeueCommand(std::string& command) {
    std::lock_guard<std::mutex> lock(commandQueueMutex); // Ensure thread-safe access to the command queue.
    if (commandQueue.empty()) {
        return false; // Return false if the queue is empty.
    }
    command = commandQueue.front(); // Get the command at the front of the queue.
    commandQueue.pop(); // Remove the command from the queue.
    return true; // Return true to indicate a command was dequeued.
}