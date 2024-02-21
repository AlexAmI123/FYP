#pragma once
#include <queue>
#include <mutex>
#include <string>
#include <thread>

class TcpListener {
public:
    TcpListener();
    ~TcpListener();
    void startListening(int port);
    bool dequeueCommand(std::string& command);

private:
    static void listenerThread(TcpListener* listener, int port);
    std::queue<std::string> commandQueue;
    std::mutex commandQueueMutex;
    std::thread* listenThread;
    void enqueueCommand(const std::string& command);
};