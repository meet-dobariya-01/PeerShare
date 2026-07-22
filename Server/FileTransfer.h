#ifndef FILETRANSFER_H
#define FILETRANSFER_H

#include <bits/stdc++.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

class FileTransfer
{
private:
    static constexpr int BUFFER_SIZE = 4096;

public:

    // Constructor
    FileTransfer();

    // Send a text message to the peer
    bool sendMessage(
        int clientSocket,
        const string& message
    );

    // Wait for ACK from peer
    bool waitForACK(
        int clientSocket
    );

    // Send a file to the peer
    bool sendFile(
        int clientSocket,
        const string& filePath
    );
};

#endif