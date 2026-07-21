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