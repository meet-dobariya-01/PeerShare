#include "FileManager.h"

// ==========================================
// Constructor
// ==========================================
FileManager::FileManager(const string& rootDirectory)
{
    this->rootDirectory = rootDirectory;
}

// ==========================================
// Check whether a topic folder exists
// ==========================================
bool FileManager::topicExists(const string& topic)
{
    string topicPath = rootDirectory + "/" + topic;

    return fs::exists(topicPath) && fs::is_directory(topicPath);
}

// ==========================================
// Create a topic directory
// ==========================================
bool FileManager::createTopicDirectory(const string& topic)
{
    string topicPath = rootDirectory + "/" + topic;
    
    try
    {
        return fs::create_directories(topicPath) || fs::exists(topicPath);
    }
    catch (...)
    {
        return false;
    }
}

// ==========================================
// Return all file names inside a topic
// ==========================================
vector<string> FileManager::getFileList(const string& topic)
{
    vector<string> fileList;

    string topicPath = rootDirectory + "/" + topic;

    if (!topicExists(topic))
    {
        return fileList;
    }

    try
    {
        for (const auto& entry : fs::directory_iterator(topicPath))
        {
            if (entry.is_regular_file())
            {
                fileList.push_back(entry.path().filename().string());
            }
        }

        sort(fileList.begin(), fileList.end());
    }
    catch (...)
    {
        fileList.clear();
    }

    return fileList;
}

// ==========================================
// Return full path of a file
// ==========================================
string FileManager::getFilePath(const string& topic,
                                const string& fileName)
{
    return rootDirectory + "/" + topic + "/" + fileName;
}

// ==========================================
// Return file size
// ==========================================
long long FileManager::getFileSize(const string& filePath)
{
    try
    {
        if (fs::exists(filePath))
        {
            return fs::file_size(filePath);
        }
    }
    catch (...)
    {
    }

    return -1;
}

// ==========================================
// Count total files inside topic
// ==========================================
int FileManager::getFileCount(const string& topic)
{
    return getFileList(topic).size();
}

// ==========================================
// Check whether file exists
// ==========================================
bool FileManager::fileExists(const string& filePath)
{
    return fs::exists(filePath) && fs::is_regular_file(filePath);
}