#pragma once
#include <netinet/in.h>
#include <string>
#include <thread>

class TcpServer {
public:
    TcpServer(int port);
    ~TcpServer();
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
