#include "TCPServer.h"
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>

TCPServer::TCPServer(int port) : port(port), serverFd(-1), clientFd(-1), running(false) {}

TCPServer::~TCPServer() {
    stop();
}

void TCPServer::start() {
    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    bind(serverFd, reinterpret_cast<sockaddr*>(&address), sizeof(address));
    listen(serverFd, 1);

    running = true;
    acceptThread = std::thread(&TCPServer::acceptConnections, this);
}

void TCPServer::stop() {
    running = false;
    if (acceptThread.joinable()) {
        acceptThread.join();
    }
    if (clientFd != -1) {
        close(clientFd);
        clientFd = -1;
    }
    if (serverFd != -1) {
        close(serverFd);
        serverFd = -1;
    }
}

void TCPServer::send(const std::vector<unsigned char>& data) {
    if (clientFd != -1) {
        ::send(clientFd, data.data(), data.size(), 0);
    }
}

void TCPServer::acceptConnections() {
    while (running) {
        clientFd = accept(serverFd, nullptr, nullptr);
        if (clientFd < 0) {
            running = false;
        }
    }
}
