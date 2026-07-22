#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <bits/stdc++.h>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

class FileManager
{
private:
    string rootDirectory;

public:
    // Constructor
    FileManager(const string& rootDirectory);

    // Check if a topic directory exists
    bool topicExists(const string& topic);

    // Return all file names inside a topic directory
    vector<string> getFileList(const string& topic);

    // Return the complete path of a file
    string getFilePath(const string& topic, const string& fileName);

    // Return file size in bytes
    long long getFileSize(const string& filePath);

    // Return total number of files in a topic
    int getFileCount(const string& topic);

    // Check if a file exists
    bool fileExists(const string& filePath);
};

#endif