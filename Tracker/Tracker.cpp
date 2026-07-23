#include <bits/stdc++.h>
#include "Tracker.h"

using namespace std;

// Constructor
Tracker::Tracker()
{
    cout << "Tracker Initialized Successfully!" << endl;
}

// Register a topic
void Tracker::registerTopic(string topicName, const Host& host)
{
    // Check if topic already exists
    if (topicDatabase.find(topicName) == topicDatabase.end())
    {
        // Create a new topic
        Topic newTopic(topicName);

        // Add host to topic
        newTopic.addHost(host);

        // Store in database
        topicDatabase[topicName] = newTopic;

        cout << "New Topic Registered : "
             << topicName << endl;
    }
    else
    {
        // Topic already exists
        topicDatabase[topicName].addHost(host);

        cout << "Host Added To Existing Topic : "
             << topicName << endl;
    }
}

// Register a chunk
void Tracker::registerChunk(string topicName, string filename, const ChunkMetadata& chunk, const Host& host)
{
    if (topicDatabase.find(topicName) == topicDatabase.end())
    {
        Topic newTopic(topicName);
        topicDatabase[topicName] = newTopic;
    }
    
    topicDatabase[topicName].registerChunk(filename, chunk, host);
    
    cout << "Chunk " << chunk.chunk_id << " registered for " << filename 
         << " under topic " << topicName << endl;
}

// Get peers for a specific chunk
vector<Host> Tracker::getPeersForChunk(string topicName, string filename, int chunk_id)
{
    if (topicDatabase.find(topicName) != topicDatabase.end())
    {
        return topicDatabase[topicName].getPeersForChunk(filename, chunk_id);
    }
    return vector<Host>();
}

// Get chunk metadata for a file
vector<ChunkMetadata> Tracker::getFileChunks(string topicName, string filename)
{
    if (topicDatabase.find(topicName) != topicDatabase.end())
    {
        return topicDatabase[topicName].getFileChunks(filename);
    }
    return vector<ChunkMetadata>();
}

// Lookup a topic
bool Tracker::lookupTopic(string topicName)
{
    auto it = topicDatabase.find(topicName);

    if (it == topicDatabase.end())
    {
        cout << "Topic Not Found!" << endl;
        return false;
    }

    cout << "\nTopic Found!\n" << endl;

    it->second.display();

    return true;
}

// Check if a topic exists
bool Tracker::topicExists(string topicName)
{
    return topicDatabase.find(topicName) != topicDatabase.end();
}

// Get hosts for a topic
vector<Host> Tracker::getHostsForTopic(string topicName)
{
    if (topicExists(topicName))
    {
        return topicDatabase[topicName].getHostList();
    }
    return vector<Host>();
}

// Display complete database
void Tracker::displayDatabase()
{
    cout << "\n==============================" << endl;
    cout << "     TRACKER DATABASE";
    cout << "\n==============================\n";

    if (topicDatabase.empty())
    {
        cout << "Database is Empty." << endl;
        return;
    }

    for (auto& entry : topicDatabase)
    {
        entry.second.display();
    }
}