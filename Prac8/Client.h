#ifndef CLIENT_H
#define CLIENT_H

#include <map>
#include <filesystem>
#include <iostream>
#include <stdexcept>

#include "FTP_Client.h"

class BackupClient
{
private:
    std::map<std::filesystem::path, std::filesystem::file_time_type> fileMap;
    std::filesystem::path directory;

public:
    BackupClient(const std::filesystem::path &directory);
    ~BackupClient();

    void updateFileMap();
    void printFileMap() const;
    bool checkForChanges();

    void backupFiles(FTPClient &ftpClient);
};

#endif