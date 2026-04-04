#include "LDAPClient.h"

LDAPClient::LDAPClient(const std::string& host, int port)
    : host(host), port(port), sockfd(-1) {
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(static_cast<uint16_t>(port));
}

LDAPClient::~LDAPClient() {
    closeConnection();
}

bool LDAPClient::connectToServer() {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "socket failed\n";
        return false;
    }

    if (inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr) <= 0) {
        std::cerr << "invalid address\n";
        close(sockfd);
        sockfd = -1;
        return false;
    }

    if (connect(sockfd, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "connect failed\n";
        close(sockfd);
        sockfd = -1;
        return false;
    }

    return true;
}

void LDAPClient::closeConnection() {
    if (sockfd >= 0) {
        close(sockfd);
        sockfd = -1;
    }
}

bool LDAPClient::sendAll(const std::vector<unsigned char>& data) {
    size_t total = 0;
    while (total < data.size()) {
        ssize_t sent = send(sockfd, data.data() + total, data.size() - total, 0);
        if (sent <= 0) {
            return false;
        }
        total += static_cast<size_t>(sent);
    }
    return true;
}

std::vector<unsigned char> LDAPClient::receivePacket() {
    unsigned char buffer[4096];
    ssize_t n = read(sockfd, buffer, sizeof(buffer));
    if (n <= 0) {
        return std::vector<unsigned char>();
    }
    return std::vector<unsigned char>(buffer, buffer + n);
}

// Very small BER-encoded anonymous bind request
std::vector<unsigned char> LDAPClient::buildAnonymousBindRequest() {
    return std::vector<unsigned char>{
        0x30, 0x0c,       // LDAPMessage sequence
        0x02, 0x01, 0x01, // messageID = 1
        0x60, 0x07,       // BindRequest
        0x02, 0x01, 0x03, // version = 3
        0x04, 0x00,       // name = ""
        0x80, 0x00        // simple auth = ""
    };
}

bool LDAPClient::anonymousBind() {
    std::vector<unsigned char> req = buildAnonymousBindRequest();
    if (!sendAll(req)) {
        return false;
    }

    std::vector<unsigned char> resp = receivePacket();
    return !resp.empty();
}

std::vector<unsigned char> LDAPClient::buildSearchRequest(const std::string& planeName)
{
    const std::string baseDN = "ou=Planes,dc=assets,dc=local";
    const std::string attributeName = "description";

    std::vector<unsigned char> searchContent;
    std::vector<unsigned char> ldapMessage;

    // baseObject
    appendOctetString(searchContent, baseDN);

    // scope = wholeSubtree (2)
    searchContent.push_back(0x0A);
    searchContent.push_back(0x01);
    searchContent.push_back(0x02);

    // derefAliases = neverDerefAliases (0)
    searchContent.push_back(0x0A);
    searchContent.push_back(0x01);
    searchContent.push_back(0x00);

    // sizeLimit = 0
    appendInteger(searchContent, 0);

    // timeLimit = 0
    appendInteger(searchContent, 0);

    // typesOnly = FALSE
    searchContent.push_back(0x01);
    searchContent.push_back(0x01);
    searchContent.push_back(0x00);

    // filter: equalityMatch (cn=planeName)
    std::vector<unsigned char> equalityMatch;
    appendOctetString(equalityMatch, "cn");
    appendOctetString(equalityMatch, planeName);
    appendSequence(searchContent, 0xA3, equalityMatch);

    // attributes: request only "description"
    std::vector<unsigned char> attributes;
    appendOctetString(attributes, attributeName);
    appendSequence(searchContent, 0x30, attributes);

    appendInteger(ldapMessage, nextMessageId++);
    appendSequence(ldapMessage, 0x63, searchContent);

    std::vector<unsigned char> finalMessage;
    appendSequence(finalMessage, 0x30, ldapMessage);

    return finalMessage;
}

std::string LDAPClient::parseSearchResultForDescription(const std::vector<unsigned char>& packet)
{
    int attrPos = findSubsequence(packet, "description", 0);
    if (attrPos < 0)
    {
        return "";
    }

    for (size_t i = static_cast<size_t>(attrPos + 11); i + 2 < packet.size(); ++i)
    {
        if (packet[i] == 0x04)
        {
            unsigned int len = packet[i + 1];

            if ((len & 0x80) == 0)
            {
                if (i + 2 + len <= packet.size())
                {
                    return std::string(packet.begin() + i + 2,
                                       packet.begin() + i + 2 + len);
                }
            }
            else
            {
                unsigned int numLenBytes = len & 0x7F;

                if (numLenBytes == 1 && i + 3 < packet.size())
                {
                    unsigned int actualLen = packet[i + 2];
                    if (i + 3 + actualLen <= packet.size())
                    {
                        return std::string(packet.begin() + i + 3,
                                           packet.begin() + i + 3 + actualLen);
                    }
                }
            }
        }
    }

    return "";
}   

std::string LDAPClient::searchPlaneSpeed(const std::string& planeName) {
    std::vector<unsigned char> req = buildSearchRequest(planeName);
    if (req.empty()) {
        return "";
    }

    if (!sendAll(req)) {
        return "";
    }

    std::vector<unsigned char> resp = receivePacket();
    if (resp.empty()) {
        return "";
    }

    return parseSearchResultForDescription(resp);
}