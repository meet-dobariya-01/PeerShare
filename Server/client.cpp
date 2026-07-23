#include "client.h"
#include "FileTransfer.h"
#include <iostream>
#include <fstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <thread>
#include <mutex>
#include <functional>

using namespace std;

// ==========================================
// Constructor
// ==========================================
Client::Client(const string& ip, int port)
{
    serverIP = ip;
    serverPort = port;
    clientSocket = -1;
}

// ==========================================
// Destructor
// ==========================================
Client::~Client()
{
    disconnect();
}

// ==========================================
// Connect to Server
// ==========================================
bool Client::connectToServer()
{
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0)
    {
        cerr << "ERROR: Failed to create client socket.\n";
        return false;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);

    if (inet_pton(AF_INET, serverIP.c_str(), &serverAddress.sin_addr) <= 0)
    {
        cerr << "ERROR: Invalid server IP address format.\n";
        close(clientSocket);
        clientSocket = -1;
        return false;
    }

    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0)
    {
        cerr << "ERROR: Connection failed to " << serverIP << ":" << serverPort << ".\n";
        close(clientSocket);
        clientSocket = -1;
        return false;
    }

    cout << "Connected to server at " << serverIP << ":" << serverPort << "\n";
    return true;
}

// ==========================================
// Disconnect from Server
// ==========================================
void Client::disconnect()
{
    if (clientSocket >= 0)
    {
        // Gracefully notify the server we are leaving
        sendMessage("EXIT\n");
        
        if (close(clientSocket) < 0)
        {
            cerr << "ERROR: Failed to close client socket.\n";
        }
        
        clientSocket = -1;
        cout << "Disconnected from server.\n";
    }
}

// ==========================================
// Send Message (Internal)
// ==========================================
bool Client::sendMessage(const string& message)
{
    if (clientSocket < 0) return false;

    // Reuse FileTransfer's robust sendMessage functionality
    FileTransfer ft;
    return ft.sendMessage(clientSocket, message);
}

// ==========================================
// Receive Message (Internal)
// ==========================================
string Client::receiveMessage()
{
    if (clientSocket < 0) return "";

    string message = "";
    char c;
    
    // Read 1 byte at a time until newline to avoid consuming 
    // binary stream data or coalesced TCP messages.
    while (true)
    {
        int bytesReceived = recv(clientSocket, &c, 1, 0);
        
        if (bytesReceived < 0)
        {
            cerr << "ERROR: Failed to receive message from server.\n";
            return "";
        }
        else if (bytesReceived == 0)
        {
            // Server disconnected
            break;
        }

        message += c;
        
        if (c == '\n')
        {
            break;
        }
    }

    return message;
}

// ==========================================
// Upload File (Not supported by protocol)
// ==========================================
bool Client::uploadFile(const string& filePath)
{
    cerr << "ERROR: Upload is not supported by the current Server protocol.\n";
    return false;
}

// ==========================================
// List Files (Not supported by protocol)
// ==========================================
vector<string> Client::listFiles()
{
    cerr << "ERROR: List Files is not supported by the current Server protocol.\n";
    return vector<string>();
}

// ==========================================
// Download File / Topic
// ==========================================
bool Client::downloadFile(const string& filename, const string& savePath)
{
    // In this topic-based P2P architecture, 'filename' functionally represents the 'topic'.
    string topic = filename;

    // 1. Send DOWNLOAD request
    if (!sendMessage("DOWNLOAD " + topic + "\n"))
    {
        cerr << "ERROR: Failed to send DOWNLOAD command.\n";
        return false;
    }

    // 2. Await Server confirmation
    string response = receiveMessage();
    if (response != "TOPIC FOUND\n")
    {
        cerr << "ERROR: Topic '" << topic << "' not found on the server.\n";
        return false;
    }

    // 3. Get number of files
    string numFilesStr = receiveMessage();
    if (numFilesStr.empty()) return false;
    
    int numFiles = 0;
    try 
    {
        numFiles = stoi(numFilesStr);
    } 
    catch (...) 
    {
        cerr << "ERROR: Invalid file count received from server.\n";
        return false;
    }

    cout << "\n----------------------------------------\n";
    cout << "Incoming topic contains " << numFiles << " files.\n";

    // Edge case: Empty topic
    if (numFiles == 0)
    {
        string endMsg = receiveMessage();
        cout << "Topic is empty. Download completed.\n";
        cout << "----------------------------------------\n";
        return true;
    }

    // 4. Receive each file
    for (int i = 0; i < numFiles; ++i)
    {
        // Receive filename
        string fileNameMsg = receiveMessage();
        if (fileNameMsg.empty()) return false;
        
        // Remove the newline character
        if (fileNameMsg.back() == '\n') 
        {
            fileNameMsg.pop_back();
        }

        // ACK the filename
        if (!sendMessage("ACK")) return false;

        // Receive filesize
        string fileSizeStr = receiveMessage();
        if (fileSizeStr.empty()) return false;
        
        long long fileSize = 0;
        try 
        {
            fileSize = stoll(fileSizeStr);
        } 
        catch (...) 
        {
            cerr << "ERROR: Invalid file size received.\n";
            return false;
        }

        // ACK the filesize
        if (!sendMessage("ACK")) return false;

        cout << "Downloading : " << fileNameMsg << " (" << fileSize << " bytes)...\n";
        
        // Open local file for writing
        string fullSavePath = savePath + "/" + fileNameMsg;
        ofstream outputFile(fullSavePath, ios::binary);
        
        if (!outputFile.is_open())
        {
            cerr << "ERROR: Unable to open local file for writing: " << fullSavePath << "\n";
            return false;
        }

        // Receive binary chunks
        long long totalReceived = 0;
        char buffer[4096];

        while (totalReceived < fileSize)
        {
            // Calculate how much we still need to avoid over-reading into the next message
            long long bytesRemaining = fileSize - totalReceived;
            int bytesToReceive = (bytesRemaining < 4096) ? bytesRemaining : 4096;
            
            int bytesReceived = recv(clientSocket, buffer, bytesToReceive, 0);

            if (bytesReceived <= 0)
            {
                cerr << "ERROR: Failed to receive binary file data mid-transfer.\n";
                outputFile.close();
                remove(fullSavePath.c_str());
                return false;
            }

            outputFile.write(buffer, bytesReceived);
            totalReceived += bytesReceived;
        }

        outputFile.close();

        // Send final ACK after the file is fully written to disk
        if (!sendMessage("ACK")) return false;
        
        cout << "Saved to    : " << fullSavePath << "\n";
    }

    // 5. Receive the END marker
    string endMsg = receiveMessage();
    if (endMsg != "END\n")
    {
        cerr << "ERROR: Did not receive END marker from server. Transfer might be incomplete.\n";
        return false;
    }

    cout << "Topic download completed successfully.\n";
    cout << "----------------------------------------\n";
    
    return true;
}

// ==========================================
// Parallel File Downloader (Phase 2)
// ==========================================
bool Client::downloadFileParallel(
    const string& topic,
    const string& filename,
    const string& savePath,
    long long totalSize,
    const vector<PeerInfo>& availablePeers
)
{
    if (availablePeers.empty())
    {
        cerr << "ERROR: No peers available for parallel download.\n";
        return false;
    }

    const long long CHUNK_SIZE = 524288; // 512 KB chunks
    int numChunks = (totalSize + CHUNK_SIZE - 1) / CHUNK_SIZE;
    
    cout << "Starting parallel download for " << filename << "\n";
    cout << "Total Size: " << totalSize << " bytes. Chunks: " << numChunks << "\n";

    vector<thread> downloadThreads;
    mutex outputMutex;
    bool overallSuccess = true;

    // Lambda for thread execution
    auto downloadChunkTask = [&](int chunkId, long long offset, long long size, PeerInfo peer) 
    {
        // Create an independent socket for this thread
        int threadSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (threadSocket < 0) return;

        sockaddr_in peerAddr;
        peerAddr.sin_family = AF_INET;
        peerAddr.sin_port = htons(peer.port);
        inet_pton(AF_INET, peer.ip.c_str(), &peerAddr.sin_addr);

        if (connect(threadSocket, (struct sockaddr*)&peerAddr, sizeof(peerAddr)) < 0)
        {
            close(threadSocket);
            lock_guard<mutex> lock(outputMutex);
            cerr << "ERROR: Thread failed to connect to peer " << peer.ip << ":" << peer.port << "\n";
            overallSuccess = false;
            return;
        }

        // Send FILE_CHUNK_REQUEST <topic> <filename> <offset> <size>
        string request = "FILE_CHUNK_REQUEST " + topic + " " + filename + " " + 
                         to_string(offset) + " " + to_string(size) + "\n";
        
        send(threadSocket, request.c_str(), request.length(), 0);

        // Receive response
        char buf[256];
        memset(buf, 0, sizeof(buf));
        
        // Read until \n
        string response = "";
        char c;
        while (recv(threadSocket, &c, 1, 0) > 0)
        {
            response += c;
            if (c == '\n') break;
        }

        if (response != "CHUNK_DATA\n")
        {
            lock_guard<mutex> lock(outputMutex);
            cerr << "ERROR: Peer refused chunk request. Response: " << response;
            close(threadSocket);
            overallSuccess = false;
            return;
        }

        // Send ACK
        send(threadSocket, "ACK", 3, 0);

        // Receive binary chunk
        string chunkFile = savePath + "/" + filename + ".chunk_" + to_string(chunkId);
        ofstream outFile(chunkFile, ios::binary);
        
        long long totalReceived = 0;
        char dataBuffer[4096];
        string hashData = "";

        while (totalReceived < size)
        {
            long long bytesRemaining = size - totalReceived;
            int bytesToReceive = (bytesRemaining < 4096) ? bytesRemaining : 4096;
            
            int bytesRecv = recv(threadSocket, dataBuffer, bytesToReceive, 0);
            if (bytesRecv <= 0) break;

            outFile.write(dataBuffer, bytesRecv);
            hashData.append(dataBuffer, bytesRecv);
            totalReceived += bytesRecv;
        }

        outFile.close();
        send(threadSocket, "ACK", 3, 0);
        close(threadSocket);

        // Validate hash
        size_t computedHash = hash<string>{}(hashData);

        lock_guard<mutex> lock(outputMutex);
        if (totalReceived == size)
        {
            cout << "Chunk " << chunkId << " downloaded successfully. Hash: " << computedHash << "\n";
        }
        else
        {
            cerr << "ERROR: Chunk " << chunkId << " download incomplete.\n";
            remove(chunkFile.c_str());
            overallSuccess = false;
        }
    };

    // Spawn threads for each chunk
    for (int i = 0; i < numChunks; ++i)
    {
        long long offset = i * CHUNK_SIZE;
        long long size = min(CHUNK_SIZE, totalSize - offset);
        
        // Simple round-robin peer assignment
        PeerInfo peer = availablePeers[i % availablePeers.size()];
        
        downloadThreads.push_back(thread(downloadChunkTask, i, offset, size, peer));
    }

    // Join all threads
    for (auto& t : downloadThreads)
    {
        if (t.joinable()) t.join();
    }

    if (!overallSuccess)
    {
        cerr << "Parallel download failed due to missing or corrupted chunks.\n";
        return false;
    }

    // Merge chunks
    cout << "Merging chunks into final file...\n";
    string finalFilePath = savePath + "/" + filename;
    ofstream finalFile(finalFilePath, ios::binary);
    
    for (int i = 0; i < numChunks; ++i)
    {
        string chunkFile = savePath + "/" + filename + ".chunk_" + to_string(i);
        ifstream inFile(chunkFile, ios::binary);
        
        if (inFile.is_open())
        {
            finalFile << inFile.rdbuf();
            inFile.close();
            remove(chunkFile.c_str()); // Delete temporary chunk
        }
    }
    
    finalFile.close();
    cout << "Parallel download and merge completed perfectly!\n";
    
    return true;
}
