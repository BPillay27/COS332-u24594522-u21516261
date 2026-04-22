#include "Client.h"

BackupClient::BackupClient(const std::filesystem::path& directory,
               const std::filesystem::path& backupDirectory)
{
    if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory))
    {
        throw std::invalid_argument("Watch directory does not exist or is not a directory.");
    }

    if (!std::filesystem::exists(backupDirectory) || !std::filesystem::is_directory(backupDirectory))
    {
        throw std::invalid_argument("Backup directory does not exist or is not a directory.");
    }

    this->directory = directory;
    this->backupDirectory = backupDirectory;
}

BackupClient::~BackupClient()
{
    // does nothing, no dynamic memory used
}

void BackupClient::updateFileMap()
{
    fileMap.clear();

    for (const auto& entry : std::filesystem::directory_iterator(directory))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".txt")
        {
            fileMap[entry.path().filename()] =
                std::filesystem::last_write_time(entry.path());
        }
    }
}

void BackupClient::printFileMap() const
{
    for (std::map<std::filesystem::path, std::filesystem::file_time_type>::const_iterator it = fileMap.begin();
         it != fileMap.end(); ++it)
    {
        std::cout << it->first << std::endl;
    }
}

bool BackupClient::checkForChanges()
{
    std::map<std::filesystem::path, std::filesystem::file_time_type> currentMap;

    for (const auto& entry : std::filesystem::directory_iterator(directory))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".txt")
        {
            currentMap[entry.path().filename()] =
                std::filesystem::last_write_time(entry.path());
        }
    }

    if (currentMap.size() != fileMap.size())
    {
        return true;
    }

    for (std::map<std::filesystem::path, std::filesystem::file_time_type>::const_iterator it = currentMap.begin();
         it != currentMap.end(); ++it)
    {
        std::map<std::filesystem::path, std::filesystem::file_time_type>::const_iterator oldIt =
            fileMap.find(it->first);

        if (oldIt == fileMap.end() || oldIt->second != it->second)
        {
            return true;
        }
    }

    return false;
}

void BackupClient::backupFiles()
{
    for (const auto& entry : std::filesystem::directory_iterator(directory))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".txt")
        {
            std::filesystem::path fileName = entry.path().filename();
            std::filesystem::file_time_type currentTime =
                std::filesystem::last_write_time(entry.path());

            std::map<std::filesystem::path, std::filesystem::file_time_type>::iterator it =
                fileMap.find(fileName);

            if (it == fileMap.end() || it->second != currentTime)
            {
                std::filesystem::copy_file(
                    entry.path(),
                    backupDirectory / fileName,
                    std::filesystem::copy_options::overwrite_existing
                );

                std::cout << "Backed up: " << fileName << std::endl;
            }
        }
    }

    updateFileMap();
}