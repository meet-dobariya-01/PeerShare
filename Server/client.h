#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <vector>

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

    vector<string> listFiles();

private:
    bool sendMessage(const string& message);
    string receiveMessage();
};

#endif