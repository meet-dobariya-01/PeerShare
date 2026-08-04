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



    // Search for a topic
    bool lookupTopic(string topicName);

    // Check if topic exists
    bool topicExists(string topicName);

    // Get hosts for a topic
    vector<Host> getHostsForTopic(string topicName);

    // Display complete database
    void displayDatabase();

};

#endif