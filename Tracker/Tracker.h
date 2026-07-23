#ifndef TRACKER_H
#define TRACKER_H

#include <bits/stdc++.h>
#include "Topic.h"

using namespace std;

class Tracker
{
private:

    // Database
    // Key   -> Topic Name
    // Value -> Topic Object
    unordered_map<string, Topic> topicDatabase;

public:

    // Constructor
    Tracker();

    // Register a topic with a host
    void registerTopic(string topicName, const Host& host);

    // Register a chunk with a host
    void registerChunk(string topicName, string filename, const ChunkMetadata& chunk, const Host& host);

    // Get peers for a specific chunk
    vector<Host> getPeersForChunk(string topicName, string filename, int chunk_id);

    // Get chunk metadata for a file
    vector<ChunkMetadata> getFileChunks(string topicName, string filename);

    // Search for a topic
    bool lookupTopic(string topicName);

    // Display complete database
    void displayDatabase();

};

#endif