#ifndef LDAPCLIENT_H
#define LDAPCLIENT_H

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

struct PlaneInfo
{
    std::string name;
    std::string speed;
};

class LDAPClient
{
private:
    std::string host;
    int port;
    int sockfd;
    struct sockaddr_in serverAddr;
    int nextMessageId;

    bool sendAll(const std::vector<unsigned char> &data);
    std::vector<unsigned char> receivePacket();

    void appendLength(std::vector<unsigned char> &out, unsigned int len);
    void appendInteger(std::vector<unsigned char> &out, int value);
    void appendOctetString(std::vector<unsigned char> &out, const std::string &value);
    void appendSequence(std::vector<unsigned char> &out, unsigned char tag, const std::vector<unsigned char> &content);

    std::vector<unsigned char> buildAnonymousBindRequest();
    std::vector<unsigned char> buildSearchRequest(const std::string &planeName);

    std::string parseSearchResultForDescription(const std::vector<unsigned char> &packet);
    int findSubsequence(const std::vector<unsigned char> &data, const std::string &text, int start) const;

    bool responseIndicatesSuccess(const std::vector<unsigned char> &response);
    bool readBERLength(const std::vector<unsigned char> &data, size_t &pos, size_t &len) const;
    bool extractPlanesFromPacket(const std::vector<unsigned char> &packet, std::vector<PlaneInfo> &planes) const;
    bool messageIsSearchResultDone(const std::vector<unsigned char> &packet) const;

public:
    LDAPClient(const std::string &host, int port);
    ~LDAPClient();
    bool simpleBind(const std::string &dn, const std::string &password);
    bool addPlane(const std::string &planeName, const std::string &speed);
    bool deletePlane(const std::string &planeName);
    bool connectToServer();
    void closeConnection();
    std::vector<PlaneInfo> parseAllPlanes(const std::vector<unsigned char> &packet);
    std::vector<PlaneInfo> getAllPlanes();

    bool anonymousBind();
    std::string searchPlaneSpeed(const std::string &planeName);
};

#endif