#pragma once
#include <queue>
#include <mutex>
#include <string>
#include <thread>

class TCPListener {
public:
    TCPListener();
    ~TCPListener();
    void startListening(uint16_t port);
    bool dequeueCommand(std::string& command);

private:
    static void listenerThread(TCPListener* listener, uint16_t port);
    std::queue<std::string> commandQueue;
    std::mutex commandQueueMutex;
    std::thread* listenThread;
    void enqueueCommand(const std::string& command);
};