#include "Host.h"
#include <bits/stdc++.h>
using namespace std;

// Default Constructor
Host::Host()
{
    ipAddress = "";
    portNumber = 0;
    directoryPath = "";
}

// Parameterized Constructor
Host::Host(const string& ip,
           int port,
           const string& path)
{
    ipAddress = ip;
    portNumber = port;
    directoryPath = path;
}

// Getter: IP Address
string Host::getIPAddress() const
{
    return ipAddress;
}

// Getter: Port Number
int Host::getPortNumber() const
{
    return portNumber;
}

// Getter: Directory Path
string Host::getDirectoryPath() const
{
    return directoryPath;
}

// Display Host Information
void Host::display() const
{
    cout << "IP Address     : " << ipAddress << endl;
    cout << "Port Number    : " << portNumber << endl;
    cout << "Directory Path : " << directoryPath << endl;
}