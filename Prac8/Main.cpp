#include "Client.h"
#include "FTP_Client.h"

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <exception>
#include <thread>
#include <chrono>

std::map<std::string, std::string> loadEnv(const std::string& filename)
{
    std::map<std::string, std::string> env;
    std::ifstream file(filename);

    if (!file.is_open())
    {
        throw std::runtime_error("Could not open .env file.");
    }

    std::string line;

    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        size_t equalPos = line.find('=');

        if (equalPos == std::string::npos)
        {
            continue;
        }

        std::string key = line.substr(0, equalPos);
        std::string value = line.substr(equalPos + 1);

        env[key] = value;
    }

    return env;
}

int main()
{
    try
    {
        std::map<std::string, std::string> env = loadEnv(".env");

        std::string watchDir = env["WATCH_DIR"];
        std::string host = env["FTP_HOST"];
        int port = std::stoi(env["FTP_PORT"]);
        std::string username = env["FTP_USER"];
        std::string password = env["FTP_PASS"];
        std::string remoteDir = env["FTP_REMOTE_DIR"];
        int pollInterval = std::stoi(env["POLL_INTERVAL"]);

        FTPClient ftpClient(host, port);

        ftpClient.connectToServer();
        ftpClient.login(username, password);
        ftpClient.changeDirectory(remoteDir);

        ftpClient.deleteRemoteFile("test.txt");

        BackupClient backupClient(watchDir);

        std::cout << "Monitoring directory: " << watchDir << std::endl;

        backupClient.backupFiles(ftpClient); // initial upload of existing .txt files

        while (true)
        {
            if (backupClient.checkForChanges())
            {
                std::cout << "Changes detected." << std::endl;
                backupClient.backupFiles(ftpClient);
            }

            std::this_thread::sleep_for(std::chrono::seconds(pollInterval));
        }
        ftpClient.disconnect();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}