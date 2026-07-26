#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <vector>
#include <mutex>
#include <unordered_set>

using namespace std;

class Client
{
private:
    int clientSocket;
    string serverIP;
    int serverPort;

public:
    Client(const string& ip, int port);
    ~Client();

    bool connectToServer();
    void disconnect();

    bool uploadFile(const string& filePath);
    bool downloadFile(const string& filename, const string& savePath);
    
    // Structure to hold peer connection details for parallel downloading
    struct PeerInfo {
        string ip;
        int port;
    };

    // Phase 2: Parallel chunk-based downloader
    bool downloadFileParallel(
        const string& topic,
        const string& filename,
        const string& savePath,
        long long totalSize,
        const vector<PeerInfo>& availablePeers
    );

    // Milestone 3.1: Query Tracker for hosts
    struct HostInfo {
        string ip;
        int port;
        string directory;
    };

    vector<HostInfo> queryTracker(const string& topic);

    // Milestone 3.2: Download an entire topic directory from a host
    bool fetchTopic(const string& topic, const string& baseSavePath);

    // Milestone 3.3: Register topic to Tracker
    bool registerTopic(const string& topic, const string& peerIP, int peerPort, const string& localDirectory);

    // Milestone 3.4: Serve topics to other peers
    void startServing(int port, const string& localDirectory);

    vector<string> listFiles();

private:
    void acceptPeerConnections(int serverSocket, const string& localDirectory);
    void handlePeerRequest(int peerSocket, const string& localDirectory);
    bool sendMessage(const string& message);
    string receiveMessage();

    // Milestone 3.5: Synchronization
    std::mutex peerMutex;
    std::unordered_set<string> downloadingTopics;
};

#endif