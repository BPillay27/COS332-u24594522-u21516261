#ifndef FTPCLIENT_H
#define FTPCLIENT_H

#include <string>
#include <filesystem>

class FTPClient
{
private:
    int controlSocket;
    std::string serverIP;
    int serverPort;

    std::string readResponse();
    void sendCommand(const std::string &command);
    int enterPassiveMode();

public:
    FTPClient(const std::string &serverIP, int serverPort);
    ~FTPClient();
    void changeDirectory(const std::string &remoteDirectory);
    void connectToServer();
    void login(const std::string &username, const std::string &password);
    void uploadFile(const std::filesystem::path &localFilePath,
                    const std::string &remoteFileName);
    long getRemoteFileSize(const std::string &remoteFileName);
    void deleteRemoteFile(const std::string &remoteFileName);
    void disconnect();
};

#endif