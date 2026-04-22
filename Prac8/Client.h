#include <map>
#include <filesystem>
#include <iostream>
#include <stdexcept>

class BackupClient
{
private:
    std::map<std::filesystem::path, std::filesystem::file_time_type> fileMap;
    std::filesystem::path directory;
    std::filesystem::path backupDirectory;

public:
    BackupClient(const std::filesystem::path &directory, const std::filesystem::path &backupDirectory);
    ~BackupClient();
    void updateFileMap();
    void printFileMap() const;
    bool checkForChanges();
    void backupFiles();
};