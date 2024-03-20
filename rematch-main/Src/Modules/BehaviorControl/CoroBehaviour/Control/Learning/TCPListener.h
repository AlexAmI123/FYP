#pragma once
#include <queue>
#include <mutex>
#include <string>
#include <thread>

// Declaration of the TCPListener class.
class TCPListener {
public:
    // Default constructor: Initializes a new instance of the TCPListener class.
    TCPListener();

    // Destructor: Responsible for cleaning up resources, such as joining threads.
    ~TCPListener();

    // Starts listening on a specified port for incoming TCP connections.
    // port: The port number on which the server will listen for incoming connections.
    void startListening(uint16_t port);

    // Attempts to dequeue a command from the internal queue of received commands.
    // command: A reference to a string where the dequeued command will be stored if available.
    // Returns true if a command was successfully dequeued, or false if the queue is empty.
    bool dequeueCommand(std::string& command);

private:
    // A static member function that represents the thread's entry point for listening to incoming connections.
    // This function is static because the thread callback needs a static function or a free-standing function.
    // listener: A pointer to the TCPListener instance that spawns the thread, used to access instance members.
    // port: The port number to listen on for incoming connections.
    static void listenerThread(TCPListener* listener, uint16_t port);

    // A queue to store the commands received from the TCP connections.
    std::queue<std::string> commandQueue;

    // A mutex to protect access to the commandQueue, ensuring thread safety.
    std::mutex commandQueueMutex;

    // A pointer to a std::thread object that will be used to run the listenerThread function in a separate thread.
    std::thread* listenThread;

    // Enqueues a command received from a TCP connection into the commandQueue.
    // This function is private because it is intended to be called only from within the listenerThread function
    // or other member functions of the TCPListener class.
    // command: The command to enqueue.
    void enqueueCommand(const std::string& command);
};