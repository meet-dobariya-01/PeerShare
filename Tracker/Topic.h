#ifndef TOPIC_H
#define TOPIC_H

#include <bits/stdc++.h>
#include "Host.h"

using namespace std;

struct ChunkMetadata {
    int chunk_id;
    long long offset;
    long long size;
    size_t hash;
};

class Topic
{
private:
    string topicName;
    vector<Host> hostList;

    // Mapping: filename -> (chunk_id -> list of peers owning this chunk)
    unordered_map<string, unordered_map<int, vector<Host>>> chunkHolders;
    
    // Mapping: filename -> complete metadata of chunks
    unordered_map<string, vector<ChunkMetadata>> fileChunks;

public:
    // Constructors
    Topic();
    Topic(string name);

    // Add a host
    void addHost(const Host& host);

    // Register a chunk to a specific host
    void registerChunk(const string& filename, const ChunkMetadata& chunk, const Host& host);

    // Get peers for a specific chunk
    vector<Host> getPeersForChunk(const string& filename, int chunk_id) const;

    // Get chunk metadata for a file
    vector<ChunkMetadata> getFileChunks(const string& filename) const;

    // Get topic name
    string getTopicName() const;

    // Get all hosts
    vector<Host> getHostList() const;

    // Display topic details
    void display() const;
};

#endif