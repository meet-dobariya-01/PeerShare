#ifndef TOPIC_H
#define TOPIC_H

#include <bits/stdc++.h>
#include "Host.h"

using namespace std;

class Topic
{
private:
    string topicName;
    vector<Host> hostList;

public:
    // Constructors
    Topic();
    Topic(string name);

    // Add a host
    void addHost(const Host& host);

    // Get topic name
    string getTopicName() const;

    // Get all hosts
    vector<Host> getHostList() const;

    // Display topic details
    void display() const;
};

#endif