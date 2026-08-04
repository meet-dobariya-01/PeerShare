#include "client.h"
#include "FileTransfer.h"
#include "FileManager.h"
#include <iostream>
#include <fstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <mutex>
#include <functional>
#include <filesystem>

namespace fs = std::filesystem;

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
// Query Tracker for Topic Hosts (Milestone 3.1)
// ==========================================
vector<Client::HostInfo> Client::queryTracker(const string& topic)
{
    vector<HostInfo> hosts;

    if (!sendMessage("QUERY " + topic + "\n"))
    {
        cerr << "ERROR: Failed to send QUERY command to Tracker.\n";
        return hosts;
    }

    string numHostsStr = receiveMessage();
    if (numHostsStr.empty())
    {
        cerr << "ERROR: Empty response from Tracker.\n";
        return hosts;
    }
    
    // Remove newline if present
    if (!numHostsStr.empty() && numHostsStr.back() == '\n') numHostsStr.pop_back();

    if (numHostsStr == "INVALID COMMAND")
    {
        cerr << "ERROR: Tracker did not recognize the QUERY command.\n";
        return hosts;
    }

    int numHosts = 0;
    try 
    {
        numHosts = stoi(numHostsStr);
    } 
    catch (...) 
    {
        cerr << "ERROR: Invalid host count received from Tracker.\n";
        return hosts;
    }

    if (numHosts == 0)
    {
        cout << "Tracker Response: 0 hosts available for topic '" << topic << "'.\n";
        return hosts;
    }

    cout << "\n----------------------------------------\n";
    cout << "Tracker Response for topic '" << topic << "':\n";
    cout << "Available Hosts: " << numHosts << "\n";

    for (int i = 0; i < numHosts; ++i)
    {
        string hostLine = receiveMessage();
        if (hostLine.empty())
        {
            cerr << "ERROR: Connection lost while reading host list.\n";
            break;
        }

        stringstream ss(hostLine);
        HostInfo info;
        if (ss >> info.ip >> info.port >> info.directory)
        {
            hosts.push_back(info);
            cout << " - " << info.ip << ":" << info.port << " [" << info.directory << "]\n";
        }
    }
    cout << "----------------------------------------\n";

    return hosts;
}

// ==========================================
// Fetch Topic from an Available Host (Milestone 3.2)
// ==========================================
bool Client::fetchTopic(const string& topic, const string& baseSavePath)
{
    // 1. Query Tracker for available hosts
    vector<HostInfo> hosts = queryTracker(topic);
    
    if (hosts.empty())
    {
        cerr << "ERROR: No hosts available for topic '" << topic << "'.\n";
        return false;
    }

    // 2. Select the first host
    HostInfo selectedHost = hosts[0];
    cout << "\nSelected Host: " << selectedHost.ip << ":" << selectedHost.port << "\n";

    // 3. Connect to the host using a temporary Client instance
    Client hostClient(selectedHost.ip, selectedHost.port);
    if (!hostClient.connectToServer())
    {
        cerr << "ERROR: Failed to connect to the selected host.\n";
        return false;
    }

    // 4. Prepare local storage using FileManager
    FileManager fm(baseSavePath);
    if (!fm.createTopicDirectory(topic))
    {
        lock_guard<mutex> lock(peerMutex);
        cerr << "ERROR: Failed to create local directory for topic storage.\n";
        hostClient.disconnect();
        return false;
    }
    
    // Mark as downloading to prevent serving incomplete files
    {
        lock_guard<mutex> lock(peerMutex);
        downloadingTopics.insert(topic);
    }
    
    // The downloadFile method will append the received filenames to this base path
    string topicPath = baseSavePath + "/" + topic;

    // 5. Request and receive the topic (reusing existing FileTransfer logic)
    {
        lock_guard<mutex> lock(peerMutex);
        cout << "Initiating download for topic '" << topic << "'...\n";
    }
    bool success = hostClient.downloadFile(topic, topicPath);
    
    {
        lock_guard<mutex> lock(peerMutex);
        downloadingTopics.erase(topic); // Remove from downloading set
        if (success) 
        {
            cout << "Successfully downloaded entire topic '" << topic << "' to " << topicPath << "\n";
        } 
        else 
        {
            cerr << "ERROR: Failed to download topic '" << topic << "'. Transfer interrupted or invalid.\n";
        }
    }

    // 6. Graceful Disconnect
    hostClient.disconnect();
    
    return success;
}

// ==========================================
// Register Topic with Tracker (Milestone 3.3)
// ==========================================
bool Client::registerTopic(const string& topic, const string& peerIP, int peerPort, const string& localDirectory)
{
    string request = "REGISTER_TOPIC " + topic + " " + peerIP + " " + to_string(peerPort) + " " + localDirectory + "\n";
    
    if (!sendMessage(request))
    {
        cerr << "ERROR: Failed to send REGISTER_TOPIC command to Tracker.\n";
        return false;
    }

    string response = receiveMessage();
    
    // Remove newline if present
    if (!response.empty() && response.back() == '\n') response.pop_back();

    if (response == "REGISTER SUCCESS")
    {
        cout << "Successfully registered topic '" << topic << "' with Tracker as host " << peerIP << ":" << peerPort << "\n";
        return true;
    }
    else
    {
        cerr << "ERROR: Failed to register topic. Tracker response: " << response << "\n";
        return false;
    }
}

// ==========================================
// Start Serving Topics to Other Peers (Milestone 3.4)
// ==========================================
void Client::startServing(int port, const string& localDirectory)
{
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0)
    {
        cerr << "ERROR: Failed to create server socket for Peer.\n";
        return;
    }

    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        cerr << "ERROR: Failed to set SO_REUSEADDR on Peer server socket.\n";
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
    {
        cerr << "ERROR: Failed to bind Peer server socket to port " << port << ".\n";
        close(serverSocket);
        return;
    }

    if (listen(serverSocket, 10) < 0)
    {
        cerr << "ERROR: Failed to listen on Peer server socket.\n";
        close(serverSocket);
        return;
    }

    cout << "\n[Peer Server] Listening for incoming peer connections on port " << port << "...\n";

    // Multi-threading: Continuously accept in a detached background thread
    thread(&Client::acceptPeerConnections, this, serverSocket, localDirectory).detach();
}

void Client::acceptPeerConnections(int serverSocket, const string& localDirectory)
{
    while (true)
    {
        sockaddr_in peerAddr;
        socklen_t peerLen = sizeof(peerAddr);
        int peerSocket = accept(serverSocket, (struct sockaddr*)&peerAddr, &peerLen);

        if (peerSocket < 0)
        {
            cerr << "ERROR: Peer failed to accept incoming connection.\n";
            continue;
        }

        // Multi-threading: Handle each requesting peer in an independent thread
        try 
        {
            thread(&Client::handlePeerRequest, this, peerSocket, localDirectory).detach();
        }
        catch (const system_error& e)
        {
            cerr << "ERROR: Peer failed to create thread: " << e.what() << "\n";
            close(peerSocket);
        }
    }
}

// Client Handling: Dedicated handler function
void Client::handlePeerRequest(int peerSocket, const string& localDirectory)
{
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    int bytesReceived = recv(peerSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived <= 0)
    {
        close(peerSocket);
        return;
    }

    string request(buffer);
    stringstream ss(request);
    string command, topic;
    
    // Reuse existing FileManager and FileTransfer modules natively
    FileTransfer fileTransfer;
    FileManager fileManager(localDirectory);

    // Receive DOWNLOAD <topic> requests
    if (ss >> command >> topic && command == "DOWNLOAD" && !topic.empty())
    {
        bool isDownloading = false;
        {
            lock_guard<mutex> lock(peerMutex);
            cout << "\n[Peer Server] Received DOWNLOAD request for topic: " << topic << "\n";
            isDownloading = (downloadingTopics.find(topic) != downloadingTopics.end());
        }
        
        // Validate that the topic exists locally and is completely downloaded
        if (!isDownloading && fileManager.topicExists(topic))
        {
            vector<string> files = fileManager.getFileList(topic);
            
            if (fileTransfer.sendMessage(peerSocket, "TOPIC FOUND\n") &&
                fileTransfer.sendMessage(peerSocket, to_string(files.size()) + "\n"))
            {
                // Send all files belonging to the requested topic
                bool transferSuccess = true;
                for (const string& filename : files)
                {
                    string filePath = fileManager.getFilePath(topic, filename);
                    long long fileSize = fileManager.getFileSize(filePath);
                    
                    if (!fileTransfer.sendMessage(peerSocket, filename + "\n") ||
                        !fileTransfer.waitForACK(peerSocket) ||
                        !fileTransfer.sendMessage(peerSocket, to_string(fileSize) + "\n") ||
                        !fileTransfer.waitForACK(peerSocket) ||
                        !fileTransfer.sendFile(peerSocket, filePath))
                    {
                        lock_guard<mutex> lock(peerMutex);
                        cerr << "[Peer Server] ERROR: Interrupted while sending file " << filename << "\n";
                        transferSuccess = false;
                        break;
                    }
                }
                
                if (transferSuccess)
                {
                    fileTransfer.sendMessage(peerSocket, "END\n");
                    lock_guard<mutex> lock(peerMutex);
                    cout << "[Peer Server] Successfully sent topic '" << topic << "'\n";
                }
            }
        }
        else
        {
            lock_guard<mutex> lock(peerMutex);
            if (isDownloading) {
                cout << "[Peer Server] Requested topic '" << topic << "' is currently downloading. Rejected.\n";
            } else {
                cout << "[Peer Server] Requested topic '" << topic << "' not found locally.\n";
            }
            fileTransfer.sendMessage(peerSocket, "TOPIC NOT FOUND\n");
        }
    }
    else
    {
        fileTransfer.sendMessage(peerSocket, "INVALID COMMAND\n");
    }

    // Close the socket after completion
    close(peerSocket);
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

        // Receive binary data
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
// Main Entry Point
// ==========================================
int main(int argc, char* argv[])
{
    string serverIP   = "127.0.0.1";
    int    serverPort = 8080;

    if (argc >= 2) serverIP   = argv[1];
    if (argc >= 3) serverPort = atoi(argv[2]);

    cout << "========================================\n";
    cout << "         PeerShare Client\n";
    cout << "========================================\n";
    cout << "Server IP   : " << serverIP   << "\n";
    cout << "Server Port : " << serverPort << "\n";
    cout << "========================================\n\n";

    Client client(serverIP, serverPort);

    if (!client.connectToServer())
    {
        cerr << "ERROR: Could not connect to server at "
             << serverIP << ":" << serverPort << "\n";
        return 1;
    }

    cout << "Connected to server.\n\n";

    string command;
    while (true)
    {
        cout << "Commands: fetch <topic> | register <topic> <port> <dir> | serve <port> <dir> | quit\n> ";
        if (!(cin >> command)) break;

        if (command == "quit" || command == "exit")
        {
            client.disconnect();
            cout << "Disconnected.\n";
            break;
        }
        else if (command == "fetch")
        {
            string topic;
            cin >> topic;
            string savePath = "../Peer/downloads";
            if (fs::exists("./Peer/downloads")) {
                savePath = "./Peer/downloads";
            } else if (fs::exists("../Peer/downloads")) {
                savePath = "../Peer/downloads";
            } else if (fs::exists("./downloads")) {
                savePath = "./downloads";
            }
            cout << "Fetching topic '" << topic << "' -> " << savePath << "/" << topic << "\n";
            if (client.fetchTopic(topic, savePath))
                cout << "Fetch complete.\n";
            else
                cerr << "Fetch failed.\n";
        }
        else if (command == "register")
        {
            string topic, localDir;
            int peerPort;
            cin >> topic >> peerPort >> localDir;
            cout << "Registering topic '" << topic << "' ...\n";
            if (client.registerTopic(topic, serverIP, peerPort, localDir))
                cout << "Registered successfully.\n";
            else
                cerr << "Registration failed.\n";
        }
        else if (command == "serve")
        {
            int port;
            string localDir;
            cin >> port >> localDir;
            cout << "Starting to serve on port " << port << " from " << localDir << " ...\n";
            client.startServing(port, localDir);
        }
        else
        {
            cerr << "Unknown command: " << command << "\n";
        }
    }

    return 0;
}
