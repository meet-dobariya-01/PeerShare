#include <bits/stdc++.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>
#include <mutex>
#include "Tracker.h"
#include "Host.h"

using namespace std;

// ==========================================
// Socket Configuration Constants
// ==========================================
constexpr int PORT = 9000;
constexpr int BUFFER_SIZE = 4096;
constexpr int BACKLOG = 10;

// Mutex for Thread Safety
mutex trackerMutex;

// ==========================================
// Forward Declarations
// ==========================================
void initializeTracker(Tracker& tracker);
void startTrackerServer(Tracker& tracker);
void acceptClients(int serverSocket, Tracker& tracker);
void handleClient(int clientSocket, Tracker& tracker);
void handleRegister(int clientSocket, stringstream& ss, Tracker& tracker);
void handleLookup(int clientSocket, stringstream& ss, Tracker& tracker);
void handleQuery(int clientSocket, stringstream& ss, Tracker& tracker);
void sendResponse(int clientSocket, const string& response);

// ==========================================
// Main Application Entry Point
// ==========================================
int main()
{
    Tracker tracker;

    initializeTracker(tracker);

    tracker.displayDatabase();

    startTrackerServer(tracker);

    return 0;
}

// ==========================================
// Initialization
// ==========================================
void initializeTracker(Tracker& tracker)
{
    Host server("127.0.0.1", 8000, "Server/raw-img/");
    Host peer1("127.0.0.1", 8001, "Peer1/downloads/");
    Host peer2("127.0.0.1", 8002, "Peer2/downloads/");
    Host peer3("127.0.0.1", 8003, "Peer3/downloads/");
    Host peer4("127.0.0.1", 8004, "Peer4/downloads/");
    Host peer5("127.0.0.1", 8005, "Peer5/downloads/");

    tracker.registerTopic("Cane", server);
    tracker.registerTopic("Cavallo", server);
    tracker.registerTopic("Elefante", server);
    tracker.registerTopic("Farfalla", server);
    tracker.registerTopic("Gallina", server);
    tracker.registerTopic("Gatto", server);
    tracker.registerTopic("Mucca", server);
    tracker.registerTopic("Pecora", server);
    tracker.registerTopic("Ragno", server);
    tracker.registerTopic("Scoiattolo", server);

    tracker.registerTopic("Cane", peer1);
    tracker.registerTopic("Cavallo", peer1);
    tracker.registerTopic("Elefante", peer1);

    tracker.registerTopic("Gatto", peer2);
    tracker.registerTopic("Mucca", peer2);
    tracker.registerTopic("Pecora", peer2);

    tracker.registerTopic("Ragno", peer3);
    tracker.registerTopic("Scoiattolo", peer3);
    tracker.registerTopic("Cane", peer3);

    tracker.registerTopic("Gallina", peer4);
    tracker.registerTopic("Farfalla", peer4);
    tracker.registerTopic("Gatto", peer4);

    tracker.registerTopic("Mucca", peer5);
    tracker.registerTopic("Elefante", peer5);
    tracker.registerTopic("Scoiattolo", peer5);
}

// ==========================================
// Networking Setup
// ==========================================
void startTrackerServer(Tracker& tracker)
{
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0)
    {
        cerr << "ERROR: Failed to call socket().\n";
        return;
    }

    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        cerr << "ERROR: Failed to call setsockopt() for SO_REUSEADDR.\n";
        if (close(serverSocket) < 0) cerr << "ERROR: Failed to close() server socket.\n";
        return;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(PORT);

    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0)
    {
        cerr << "ERROR: Failed to call bind() on port " << PORT << ".\n";
        if (close(serverSocket) < 0) cerr << "ERROR: Failed to close() server socket.\n";
        return;
    }

    if (listen(serverSocket, BACKLOG) < 0)
    {
        cerr << "ERROR: Failed to call listen() on server socket.\n";
        if (close(serverSocket) < 0) cerr << "ERROR: Failed to close() server socket.\n";
        return;
    }

    cout << "====================================================\n";
    cout << "            PeerShare Tracker\n";
    cout << "====================================================\n";
    cout << "Status : Running\n";
    cout << "Port   : " << PORT << "\n";
    cout << "Mode   : Multi-threaded\n";
    cout << "Waiting for Peer Connections...\n";
    cout << "====================================================\n";

    acceptClients(serverSocket, tracker);

    if (close(serverSocket) < 0)
    {
        cerr << "ERROR: Failed to close() server socket upon exit.\n";
    }
}

// ==========================================
// Client Acceptance
// ==========================================
void acceptClients(int serverSocket, Tracker& tracker)
{
    while (true)
    {
        sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientSocket < 0)
        {
            cerr << "ERROR: Failed to call accept() for incoming connection.\n";
            continue;
        }

        // Retrieve IP and Port of connected client
        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
        int clientPort = ntohs(clientAddr.sin_port);

        cout << "Peer Connected\n";
        cout << "IP : " << clientIP << "\n";
        cout << "Port : " << clientPort << "\n";

        // Dispatch client to its own detached thread
        thread clientThread(handleClient, clientSocket, ref(tracker));
        clientThread.detach();
    }
}

// ==========================================
// Client Handling
// ==========================================
void handleClient(int clientSocket, Tracker& tracker)
{
    char buffer[BUFFER_SIZE];

    while (true)
    {
        memset(buffer, 0, BUFFER_SIZE);
        
        int bytesRead = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytesRead < 0)
        {
            cerr << "ERROR: Failed to call recv() from client socket.\n";
            break;
        }
        else if (bytesRead == 0)
        {
            // Client graceful disconnect
            break; 
        }

        string request(buffer);
        stringstream ss(request);
        string command;
        
        // If stringstream fails to extract a command, ignore and wait for next message
        if (!(ss >> command)) continue;

        if (command == "REGISTER")
        {
            handleRegister(clientSocket, ss, tracker);
        }
        else if (command == "LOOKUP")
        {
            handleLookup(clientSocket, ss, tracker);
        }
        else if (command == "QUERY")
        {
            handleQuery(clientSocket, ss, tracker);
        }
        else if (command == "EXIT")
        {
            break;
        }
        else
        {
            sendResponse(clientSocket, "INVALID COMMAND\n");
        }
    }

    cout << "Peer Disconnected\n";
    
    if (close(clientSocket) < 0)
    {
        cerr << "ERROR: Failed to close() client socket.\n";
    }
}

// ==========================================
// Command Handlers
// ==========================================
void handleRegister(int clientSocket, stringstream& ss, Tracker& tracker)
{
    string topic, ip, directory;
    int port;
    
    if (ss >> topic >> ip >> port >> directory)
    {
        Host newHost(ip, port, directory);
        
        // Lock only when interacting with the Tracker API
        {
            lock_guard<mutex> lock(trackerMutex);
            tracker.registerTopic(topic, newHost);
        }

        cout << "------------------------------------\n";
        cout << "Peer Registered\n";
        cout << "Topic : " << topic << "\n";
        cout << "IP : " << ip << "\n";
        cout << "Port : " << port << "\n";
        cout << "Directory : " << directory << "\n";
        cout << "------------------------------------\n";

        sendResponse(clientSocket, "REGISTER SUCCESS\n");
    }
    else
    {
        sendResponse(clientSocket, "INVALID COMMAND\n");
    }
}

void handleLookup(int clientSocket, stringstream& ss, Tracker& tracker)
{
    string topic;
    
    if (ss >> topic)
    {
        bool exists = false;
        vector<Host> hosts;
        
        // Lock only when interacting with the Tracker API
        {
            lock_guard<mutex> lock(trackerMutex);
            
            // Calling assumed clean Tracker public APIs
            exists = tracker.topicExists(topic);
            if (exists)
            {
                hosts = tracker.getHostsForTopic(topic);
            }
        }
        
        if (exists)
        {
            string response = "TOPIC FOUND\n";
            for (const Host& h : hosts)
            {
                response += h.getIPAddress() + " " + to_string(h.getPortNumber()) + " " + h.getDirectoryPath() + "\n";
            }
            sendResponse(clientSocket, response);
        }
        else
        {
            sendResponse(clientSocket, "TOPIC NOT FOUND\n");
        }
    }
    else
    {
        sendResponse(clientSocket, "INVALID COMMAND\n");
    }
}

// ==========================================
// Network Utilities
// ==========================================
void sendResponse(int clientSocket, const string& response)
{
    if (send(clientSocket, response.c_str(), response.length(), 0) < 0)
    {
        cerr << "ERROR: Failed to call send() to client socket.\n";
    }
}

void handleQuery(int clientSocket, stringstream& ss, Tracker& tracker)
{
    string topic;
    
    if (ss >> topic)
    {
        bool exists = false;
        vector<Host> hosts;
        
        {
            lock_guard<mutex> lock(trackerMutex);
            exists = tracker.topicExists(topic);
            if (exists)
            {
                hosts = tracker.getHostsForTopic(topic);
            }
        }
        
        if (exists && !hosts.empty())
        {
            string response = to_string(hosts.size()) + "\n";
            for (const Host& h : hosts)
            {
                response += h.getIPAddress() + " " + to_string(h.getPortNumber()) + " " + h.getDirectoryPath() + "\n";
            }
            sendResponse(clientSocket, response);
        }
        else
        {
            sendResponse(clientSocket, "0\n");
        }
    }
    else
    {
        sendResponse(clientSocket, "INVALID COMMAND\n");
    }
}