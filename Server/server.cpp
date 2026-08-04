#include "Server.h"

// ==========================================
// Constructor
// ==========================================
Server::Server(
    int port,
    const string& rootDirectory
)
    : port(port),
      rootDirectory(rootDirectory),
      fileManager(rootDirectory)
{
    serverSocket = -1;
}

// ==========================================
// Destructor
// ==========================================
Server::~Server()
{
    shutdownServer();
}

// ==========================================
// Start Server
// ==========================================
void Server::start()
{
    if (!createSocket())
    {
        return;
    }

    if (!bindSocket())
    {
        return;
    }

    if (!listenForPeers())
    {
        return;
    }

    cout << "========================================\n";
    cout << "        PeerShare Server\n";
    cout << "========================================\n";
    cout << "Status : Running\n";
    cout << "Port   : " << port << endl;
    cout << "Waiting for Peer Connections...\n";
    cout << "========================================\n";

    acceptClients();
}

// ==========================================
// Create Socket
// ==========================================
bool Server::createSocket()
{
    serverSocket = socket(
        AF_INET,
        SOCK_STREAM,
        0
    );

    if (serverSocket < 0)
    {
        cerr << "ERROR: socket() failed.\n";
        return false;
    }

    int option = 1;

    if (setsockopt(
            serverSocket,
            SOL_SOCKET,
            SO_REUSEADDR,
            &option,
            sizeof(option)
        ) < 0)
    {
        cerr << "ERROR: setsockopt() failed.\n";

        if (close(serverSocket) < 0) 
        {
            cerr << "ERROR: close() failed after setsockopt error.\n";
        }
        serverSocket = -1;

        return false;
    }

    return true;
}

// ==========================================
// Bind Socket
// ==========================================
bool Server::bindSocket()
{
    sockaddr_in serverAddress;

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port);

    if (bind(
            serverSocket,
            (sockaddr*)&serverAddress,
            sizeof(serverAddress)
        ) < 0)
    {
        cerr << "ERROR: bind() failed.\n";

        if (close(serverSocket) < 0) 
        {
            cerr << "ERROR: close() failed after bind error.\n";
        }
        serverSocket = -1;

        return false;
    }

    return true;
}

// ==========================================
// Listen For Peers
// ==========================================
bool Server::listenForPeers()
{
    if (listen(serverSocket, BACKLOG) < 0)
    {
        cerr << "ERROR: listen() failed.\n";

        if (close(serverSocket) < 0) 
        {
            cerr << "ERROR: close() failed after listen error.\n";
        }
        serverSocket = -1;

        return false;
    }

    return true;
}

// ==========================================
// Accept Incoming Peer Connections
// ==========================================
void Server::acceptClients()
{
    while (true)
    {
        sockaddr_in clientAddress;
        socklen_t clientLength = sizeof(clientAddress);

        int clientSocket = accept(
            serverSocket,
            (sockaddr*)&clientAddress,
            &clientLength
        );

        if (clientSocket < 0)
        {
            lock_guard<mutex> lock(serverMutex);
            cerr << "ERROR: accept() failed.\n";
            continue;
        }

        char clientIP[INET_ADDRSTRLEN];

        inet_ntop(
            AF_INET,
            &clientAddress.sin_addr,
            clientIP,
            INET_ADDRSTRLEN
        );

        {
            lock_guard<mutex> lock(serverMutex);
            cout << "\n----------------------------------------\n";
            cout << "Peer Connected\n";
            cout << "IP   : " << clientIP << endl;
            cout << "Port : " << ntohs(clientAddress.sin_port) << endl;
            cout << "----------------------------------------\n";
        }

        try
        {
            thread peerThread(
                &Server::handlePeer,
                this,
                clientSocket
            );

            peerThread.detach();
        }
        catch (const system_error& e)
        {
            {
                lock_guard<mutex> lock(serverMutex);
                cerr << "ERROR: Failed to create thread: " << e.what() << "\n";
            }
            close(clientSocket);
        }
    }
}

// ==========================================
// Handle One Connected Peer
// ==========================================
void Server::handlePeer(int clientSocket)
{
    char buffer[BUFFER_SIZE];

    // Retrieve Peer IP and Port for enhanced logging
    sockaddr_in peerAddr;
    socklen_t peerLen = sizeof(peerAddr);
    string peerIP = "Unknown";
    int peerPort = 0;

    if (getpeername(clientSocket, (struct sockaddr*)&peerAddr, &peerLen) == 0)
    {
        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &peerAddr.sin_addr, ipStr, INET_ADDRSTRLEN);
        peerIP = ipStr;
        peerPort = ntohs(peerAddr.sin_port);
    }

    while (true)
    {
        memset(buffer, 0, BUFFER_SIZE);

        int bytesReceived = recv(
            clientSocket,
            buffer,
            BUFFER_SIZE - 1,
            0
        );

        // Separate handling of network errors from graceful disconnects
        if (bytesReceived < 0)
        {
            lock_guard<mutex> lock(serverMutex);
            cerr << "ERROR: recv() failed for peer " << peerIP << ":" << peerPort << "\n";
            break;
        }
        else if (bytesReceived == 0)
        {
            // Graceful disconnect
            break;
        }

        string request(buffer);
        stringstream ss(request);

        string command;
        if (!(ss >> command))
        {
            continue; // Ignore empty lines gracefully
        }

        if (command == "DOWNLOAD")
        {
            string topic;
            
            // Validate DOWNLOAD command requires a non-empty topic
            if (ss >> topic && !topic.empty())
            {
                {
                    lock_guard<mutex> lock(serverMutex);
                    cout << "\nRequested Topic : " << topic << endl;
                    cout << "Client IP and Port: " << peerIP << ":" << peerPort << endl;
                }

                sendTopic(
                    clientSocket,
                    topic
                );
            }
            else
            {
                if (!fileTransfer.sendMessage(clientSocket, "INVALID COMMAND\n")) 
                {
                    break;
                }
            }
        }
        else if (command == "EXIT")
        {
            break;
        }
        else
        {
            if (!fileTransfer.sendMessage(clientSocket, "INVALID COMMAND\n")) 
            {
                break;
            }
        }
    }

    {
        lock_guard<mutex> lock(serverMutex);
        cout << "Peer Disconnected (IP: " << peerIP << " Port: " << peerPort << ")\n";
    }

    if (close(clientSocket) < 0)
    {
        lock_guard<mutex> lock(serverMutex);
        cerr << "ERROR: Failed to close client socket.\n";
    }
}

// ==========================================
// Send Complete Topic
// ==========================================
void Server::sendTopic(
    int clientSocket,
    const string& topic
)
{
    // Check whether topic exists
    if (!fileManager.topicExists(topic))
    {
        if (!fileTransfer.sendMessage(clientSocket, "TOPIC NOT FOUND\n")) 
        {
            lock_guard<mutex> lock(serverMutex);
            cout << "Transfer Failed\n";
        }
        return;
    }

    // Inform peer that topic exists
    if (!fileTransfer.sendMessage(clientSocket, "TOPIC FOUND\n"))
    {
        lock_guard<mutex> lock(serverMutex);
        cout << "Transfer Failed\n";
        return;
    }

    // Get all files
    vector<string> files = fileManager.getFileList(topic);

    // Topic exists but contains zero files edge case
    if (files.empty())
    {
        {
            lock_guard<mutex> lock(serverMutex);
            cout << "Topic '" << topic << "' has 0 files. Sending empty directory response.\n";
        }
        
        if (!fileTransfer.sendMessage(clientSocket, "0\n"))
        {
            lock_guard<mutex> lock(serverMutex);
            cout << "Transfer Failed\n";
            return;
        }
        
        if (!fileTransfer.sendMessage(clientSocket, "END\n"))
        {
            lock_guard<mutex> lock(serverMutex);
            cout << "Transfer Failed\n";
            return;
        }

        {
            lock_guard<mutex> lock(serverMutex);
            cout << "Transfer Completed\n";
        }
        return;
    }

    {
        lock_guard<mutex> lock(serverMutex);
        cout << "Number of Files : " << files.size() << endl;
    }

    // Send number of files
    if (!fileTransfer.sendMessage(clientSocket, to_string(files.size()) + "\n"))
    {
        lock_guard<mutex> lock(serverMutex);
        cout << "Transfer Failed\n";
        return;
    }

    // Send every file, catching internal aborts gracefully
    try
    {
        for (const string& fileName : files)
        {
            sendSingleFile(
                clientSocket,
                topic,
                fileName
            );
        }
    }
    catch (const runtime_error& e)
    {
        lock_guard<mutex> lock(serverMutex);
        cout << "Transfer Failed\n";
        return;
    }

    // Notify completion
    if (!fileTransfer.sendMessage(clientSocket, "END\n"))
    {
        lock_guard<mutex> lock(serverMutex);
        cout << "Transfer Failed\n";
        return;
    }

    {
        lock_guard<mutex> lock(serverMutex);
        cout << "Transfer Completed\n";
    }
}

// ==========================================
// Send One File
// ==========================================
void Server::sendSingleFile(
    int clientSocket,
    const string& topic,
    const string& fileName
)
{
    string filePath = fileManager.getFilePath(topic, fileName);
    long long fileSize = fileManager.getFileSize(filePath);

    {
        lock_guard<mutex> lock(serverMutex);
        cout << "----------------------------------------\n";
        cout << "Current File\n";
        cout << "Sending : " << fileName << endl;
        cout << "Filename : " << fileName << endl;
        cout << "Size : " << fileSize << " bytes\n";
        cout << "----------------------------------------\n";
    }

    // Send filename
    if (!fileTransfer.sendMessage(clientSocket, fileName + "\n"))
    {
        throw runtime_error("Failed to send filename");
    }

    // Wait for ACK
    if (!fileTransfer.waitForACK(clientSocket))
    {
        throw runtime_error("Failed to receive ACK for filename");
    }

    // Send filesize
    if (!fileTransfer.sendMessage(clientSocket, to_string(fileSize) + "\n"))
    {
        throw runtime_error("Failed to send filesize");
    }

    // Wait for ACK
    if (!fileTransfer.waitForACK(clientSocket))
    {
        throw runtime_error("Failed to receive ACK for filesize");
    }

    // Send binary file
    if (!fileTransfer.sendFile(clientSocket, filePath))
    {
        throw runtime_error("Failed to send file binary data");
    }
}

// ==========================================
// Shutdown Server
// ==========================================
void Server::shutdownServer()
{
    if (serverSocket >= 0)
    {
        if (close(serverSocket) < 0)
        {
            lock_guard<mutex> lock(serverMutex);
            cerr << "ERROR: Failed to close server socket.\n";
        }
        
        serverSocket = -1; // Reset to avoid double-close issues

        {
            lock_guard<mutex> lock(serverMutex);
            cout << "\nServer Shutdown\n";
        }
    }
}