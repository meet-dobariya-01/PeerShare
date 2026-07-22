#ifndef SERVER_H
#define SERVER_H

#include <bits/stdc++.h>
#include <thread>
#include <mutex>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include "FileManager.h"
#include "FileTransfer.h"

using namespace std;

class Server
{
private:

    // Socket Information
    int serverSocket;
    int port;

    // Root Dataset Directory
    string rootDirectory;

    // Helper Classes
    FileManager fileManager;
    FileTransfer fileTransfer;

    // Thread Safety
    mutex serverMutex;

    // Configuration
    static constexpr int BUFFER_SIZE = 4096;
    static constexpr int BACKLOG = 10;

public:

    // Constructor
    Server(
        int port,
        const string& rootDirectory
    );

    // Destructor
    ~Server();

    // Start the server
    void start();

private:

    // Create socket
    bool createSocket();

    // Bind socket
    bool bindSocket();

    // Listen for peers
    bool listenForPeers();

    // Accept incoming peers
    void acceptClients();

    // Handle one peer
    void handlePeer(
        int clientSocket
    );

    // Send an entire topic
    void sendTopic(
        int clientSocket,
        const string& topic
    );

    // Send one file
    void sendSingleFile(
        int clientSocket,
        const string& topic,
        const string& fileName
    );

    // Close server socket
    void shutdownServer();
};

#endif