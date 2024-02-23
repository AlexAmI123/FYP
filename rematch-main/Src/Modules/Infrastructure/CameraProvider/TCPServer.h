#pragma once
#include <netinet/in.h>
#include <string>
#include <thread>

class TCPServer {
public:
    TCPServer(int port);
    ~TCPServer();
    void start();
    void stop();
    void send(const std::vector<unsigned char>& data);

private:
    int port;
    int serverFd;
    int clientFd;
    std::thread acceptThread;
    bool running;

    void acceptConnections();
};
