#include "TCPListener.h"
#include <netinet/in.h>
#include <cstring>
#include <unistd.h>
#include <iostream>

TCPListener::TCPListener() : listenThread(nullptr) {}

TCPListener::~TCPListener() {
    if (listenThread && listenThread->joinable()) {
        listenThread->join();
    }
    delete listenThread;
}

void TCPListener::startListening(int port) {
    listenThread = new std::thread(listenerThread, this, port);
}

void TCPListener::listenerThread(TCPListener* listener, int port) {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    if ((client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    while (read(client_socket, buffer, 1024) > 0) {
        listener->enqueueCommand(std::string(buffer));
        memset(buffer, 0, 1024);
    }

    close(server_fd);
}

void TCPListener::enqueueCommand(const std::string& command) {
    std::lock_guard<std::mutex> lock(commandQueueMutex);
    commandQueue.push(command);
}

bool TCPListener::dequeueCommand(std::string& command) {
    std::lock_guard<std::mutex> lock(commandQueueMutex);
    if (commandQueue.empty()) {
        return false;
    }
    command = commandQueue.front();
    commandQueue.pop();
    return true;
}
