#include "FTP_Client.h"

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <cstring>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

FTPClient::FTPClient(const std::string &serverIP, int serverPort)
{
    this->serverIP = serverIP;
    this->serverPort = serverPort;
    this->controlSocket = -1;
}

FTPClient::~FTPClient()
{
    disconnect();
}

std::string FTPClient::readResponse()
{
    char buffer[4096];
    std::memset(buffer, 0, sizeof(buffer));

    int bytesRead = recv(controlSocket, buffer, sizeof(buffer) - 1, 0);

    if (bytesRead <= 0)
    {
        throw std::runtime_error("Failed to read FTP response.");
    }

    std::string response(buffer);

    std::cout << "SERVER: " << response;

    return response;
}

void FTPClient::sendCommand(const std::string &command)
{
    std::string fullCommand = command + "\r\n";

    int bytesSent = send(controlSocket, fullCommand.c_str(), fullCommand.length(), 0);

    if (bytesSent < 0)
    {
        throw std::runtime_error("Failed to send FTP command.");
    }

    std::cout << "CLIENT: " << command << std::endl;
}

void FTPClient::connectToServer()
{
    controlSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (controlSocket < 0)
    {
        throw std::runtime_error("Failed to create control socket.");
    }

    sockaddr_in serverAddress;
    std::memset(&serverAddress, 0, sizeof(serverAddress));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);

    if (inet_pton(AF_INET, serverIP.c_str(), &serverAddress.sin_addr) <= 0)
    {
        throw std::runtime_error("Invalid server IP address.");
    }

    if (connect(controlSocket, (sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        throw std::runtime_error("Failed to connect to FTP server.");
    }

    readResponse(); // 220 greeting
}

void FTPClient::login(const std::string &username, const std::string &password)
{
    sendCommand("USER " + username);
    readResponse();

    sendCommand("PASS " + password);
    readResponse();

    sendCommand("TYPE I");
    readResponse();
}

void FTPClient::disconnect()
{
    if (controlSocket != -1)
    {
        sendCommand("QUIT");

        try
        {
            readResponse();
        }
        catch (...)
        {
            // Ignore errors during shutdown
        }

        close(controlSocket);
        controlSocket = -1;
    }
}

int FTPClient::enterPassiveMode()
{
    sendCommand("EPSV");

    std::string response = readResponse();

    if (response.substr(0, 3) != "229")
    {
        throw std::runtime_error("EPSV command failed.");
    }

    // Example response:
    // 229 Entering Extended Passive Mode (|||32051|)
    size_t firstPipe = response.find('|');
    size_t lastPipe = response.rfind('|');

    if (firstPipe == std::string::npos || lastPipe == std::string::npos || firstPipe == lastPipe)
    {
        throw std::runtime_error("Could not parse EPSV response.");
    }

    std::string portString = response.substr(firstPipe + 3, lastPipe - (firstPipe + 3));

    return std::stoi(portString);
}

void FTPClient::uploadFile(const std::filesystem::path &localFilePath, const std::string &remoteFileName)
{
    std::ifstream file(localFilePath, std::ios::binary);

    if (!file.is_open())
    {
        throw std::runtime_error("Could not open local file for upload.");
    }

    int dataPort = enterPassiveMode();

    int dataSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (dataSocket < 0)
    {
        throw std::runtime_error("Failed to create data socket.");
    }

    sockaddr_in dataAddress;
    std::memset(&dataAddress, 0, sizeof(dataAddress));

    dataAddress.sin_family = AF_INET;
    dataAddress.sin_port = htons(dataPort);

    if (inet_pton(AF_INET, serverIP.c_str(), &dataAddress.sin_addr) <= 0)
    {
        close(dataSocket);
        throw std::runtime_error("Invalid FTP server IP address for data connection.");
    }

    if (connect(dataSocket, (sockaddr *)&dataAddress, sizeof(dataAddress)) < 0)
    {
        close(dataSocket);
        throw std::runtime_error("Failed to connect data socket.");
    }

    sendCommand("STOR " + remoteFileName);

    std::string response = readResponse();

    if (response.substr(0, 3) != "150")
    {
        close(dataSocket);
        throw std::runtime_error("FTP server refused file upload.");
    }

    char buffer[4096];

    while (file.good())
    {
        file.read(buffer, sizeof(buffer));
        std::streamsize bytesRead = file.gcount();

        if (bytesRead > 0)
        {
            int bytesSent = send(dataSocket, buffer, bytesRead, 0);

            if (bytesSent < 0)
            {
                close(dataSocket);
                throw std::runtime_error("Failed to send file data.");
            }
        }
    }

    close(dataSocket);

    response = readResponse();

    if (response.substr(0, 3) != "226")
    {
        throw std::runtime_error("File upload did not complete successfully.");
    }

    std::cout << "Uploaded file: " << remoteFileName << std::endl;
    long remoteSize = getRemoteFileSize(remoteFileName);

    std::cout << "Remote file size: " << remoteSize << " bytes" << std::endl;
}

void FTPClient::changeDirectory(const std::string &remoteDirectory)
{
    sendCommand("CWD " + remoteDirectory);

    std::string response = readResponse();

    if (response.substr(0, 3) != "250")
    {
        throw std::runtime_error("Failed to change FTP remote directory.");
    }
}

long FTPClient::getRemoteFileSize(const std::string &remoteFileName)
{
    sendCommand("SIZE " + remoteFileName);

    std::string response = readResponse();

    if (response.substr(0, 3) != "213")
    {
        throw std::runtime_error("Failed to get remote file size.");
    }
    std::string sizeString = response.substr(4);

    return std::stol(sizeString);
}

void FTPClient::deleteRemoteFile(const std::string &remoteFileName)
{
    sendCommand("DELE " + remoteFileName);

    std::string response = readResponse();

    if (response.substr(0, 3) != "250")
    {
        throw std::runtime_error("Failed to delete remote file.");
    }

    std::cout << "Deleted remote file: " << remoteFileName << std::endl;
}