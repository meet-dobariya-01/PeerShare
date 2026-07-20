#ifndef HOST_H
#define HOST_H

#include <bits/stdc++.h>
using namespace std;

class Host
{
private:
    string ipAddress;
    int portNumber;
    string directoryPath;

public:
    // Constructor
    Host();

    Host(const string& ip,
         int port,
         const string& path);

    // Getters
    string getIPAddress() const;

    int getPortNumber() const;

    string getDirectoryPath() const;

    // Display host information
    void display() const;
};

#endif