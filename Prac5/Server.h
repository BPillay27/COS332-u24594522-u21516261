#ifndef LDAPCLIENT_H
#define LDAPCLIENT_H

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

class LDAPClient {
public:
    LDAPClient(const std::string& host, int port);
    ~LDAPClient();

    bool connectToServer();
    void closeConnection();

    bool anonymousBind();
    std::string searchPlaneSpeed(const std::string& planeName);

private:
    std::string host;
    int port;
    int sockfd;
    sockaddr_in serverAddr;

    bool sendAll(const std::vector<unsigned char>& data);
    std::vector<unsigned char> readPacket();

    std::vector<unsigned char> buildAnonymousBindRequest();
    std::vector<unsigned char> buildSearchRequest(const std::string& planeName);

    std::string parseSearchResultForDescription(const std::vector<unsigned char>& packet);
};

#endif