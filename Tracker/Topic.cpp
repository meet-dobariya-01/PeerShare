#include <bits/stdc++.h>
#include "Topic.h"

using namespace std;

// Default Constructor
Topic::Topic()
{
    topicName = "";
}

// Parameterized Constructor
Topic::Topic(string name)
{
    topicName = name;
}

// Add a new host to the topic
void Topic::addHost(const Host& host)
{
    hostList.push_back(host);
}

// Register a specific chunk to a host
void Topic::registerChunk(const string& filename, const ChunkMetadata& chunk, const Host& host)
{
    // Save metadata if not already saved for this chunk
    if (fileChunks.find(filename) == fileChunks.end()) {
        fileChunks[filename] = vector<ChunkMetadata>();
    }
    
    // Check if chunk metadata already exists
    bool exists = false;
    for (const auto& c : fileChunks[filename]) {
        if (c.chunk_id == chunk.chunk_id) {
            exists = true;
            break;
        }
    }
    if (!exists) {
        fileChunks[filename].push_back(chunk);
    }

    // Assign host to this chunk
    chunkHolders[filename][chunk.chunk_id].push_back(host);
}

// Get peers for a specific chunk
vector<Host> Topic::getPeersForChunk(const string& filename, int chunk_id) const
{
    auto fileIt = chunkHolders.find(filename);
    if (fileIt != chunkHolders.end()) {
        auto chunkIt = fileIt->second.find(chunk_id);
        if (chunkIt != fileIt->second.end()) {
            return chunkIt->second;
        }
    }
    return vector<Host>();
}

// Get chunk metadata for a file
vector<ChunkMetadata> Topic::getFileChunks(const string& filename) const
{
    auto it = fileChunks.find(filename);
    if (it != fileChunks.end()) {
        return it->second;
    }
    return vector<ChunkMetadata>();
}

// Return topic name
string Topic::getTopicName() const
{
    return topicName;
}

// Return all hosts
vector<Host> Topic::getHostList() const
{
    return hostList;
}

// Display topic information
void Topic::display() const
{
    cout << "--------------------------------" << endl;
    cout << "Topic Name : " << topicName << endl;
    cout << "Available Hosts :" << endl;

    for (const Host& host : hostList)
    {
        host.display();
        cout << "----------------------------" << endl;
    }
}