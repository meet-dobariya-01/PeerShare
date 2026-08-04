#include "FileTransfer.h"

// ==========================================
// Constructor
// ==========================================
FileTransfer::FileTransfer()
{
}

// ==========================================
// Send a text message
// ==========================================
bool FileTransfer::sendMessage(
    int clientSocket,
    const string& message
)
{
    int totalSent = 0;
    int length = message.length();
    const char* data = message.c_str();

    while (totalSent < length)
    {
        int bytesSent = send(
            clientSocket,
            data + totalSent,
            length - totalSent,
            0
        );

        if (bytesSent < 0)
        {
            cerr << "ERROR: Failed to send message.\n";
            return false;
        }

        totalSent += bytesSent;
    }

    return true;
}

// ==========================================
// Wait for ACK from peer
// ==========================================
bool FileTransfer::waitForACK(
    int clientSocket
)
{
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    int bytesReceived = recv(
        clientSocket,
        buffer,
        BUFFER_SIZE - 1,
        0
    );

    if (bytesReceived <= 0)
    {
        cerr << "ERROR: Failed to receive ACK.\n";
        return false;
    }

    string response(buffer);

    if (response == "ACK")
    {
        return true;
    }

    cerr << "ERROR: Invalid ACK received.\n";
    return false;
}

// ==========================================
// Send a file
// ==========================================
bool FileTransfer::sendFile(
    int clientSocket,
    const string& filePath
)
{
    ifstream inputFile(
        filePath,
        ios::binary
    );

    if (!inputFile.is_open())
    {
        cerr << "ERROR: Unable to open file : "
             << filePath
             << endl;

        return false;
    }

    char buffer[BUFFER_SIZE];

    while (true)
    {
        inputFile.read(
            buffer,
            BUFFER_SIZE
        );

        streamsize bytesRead =
            inputFile.gcount();

        if (bytesRead <= 0)
        {
            break;
        }

        int totalSent = 0;
        while (totalSent < bytesRead)
        {
            int bytesSent = send(
                clientSocket,
                buffer + totalSent,
                bytesRead - totalSent,
                0
            );

            if (bytesSent < 0)
            {
                cerr << "ERROR: Failed to send file data.\n";
                inputFile.close();
                return false;
            }

            totalSent += bytesSent;
        }
    }

    inputFile.close();

    return waitForACK(clientSocket);
}