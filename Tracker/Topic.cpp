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
    // Prevent duplicate registrations
    for (const Host& h : hostList) {
        if (h.getIPAddress() == host.getIPAddress() && 
            h.getPortNumber() == host.getPortNumber() && 
            h.getDirectoryPath() == host.getDirectoryPath()) {
            return; // Already registered
        }
    }
    hostList.push_back(host);
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