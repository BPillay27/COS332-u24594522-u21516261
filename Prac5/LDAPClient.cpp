#include "LDAPClient.h"

LDAPClient::LDAPClient(const std::string& host, int port)
    : host(host), port(port), sockfd(-1), nextMessageId(1)
{
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(static_cast<unsigned short>(port));
}

LDAPClient::~LDAPClient()
{
    closeConnection();
}

bool LDAPClient::connectToServer()
{
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        std::cerr << "Socket creation failed: " << std::strerror(errno) << std::endl;
        return false;
    }

    if (inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr) <= 0)
    {
        std::cerr << "Invalid address: " << host << std::endl;
        close(sockfd);
        sockfd = -1;
        return false;
    }

    if (connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
    {
        std::cerr << "Connect failed: " << std::strerror(errno) << std::endl;
        close(sockfd);
        sockfd = -1;
        return false;
    }

    return true;
}

void LDAPClient::closeConnection()
{
    if (sockfd >= 0)
    {
        close(sockfd);
        sockfd = -1;
    }
}

bool LDAPClient::sendAll(const std::vector<unsigned char>& data)
{
    size_t totalSent = 0;

    while (totalSent < data.size())
    {
        ssize_t sent = send(sockfd,
                            reinterpret_cast<const char*>(&data[totalSent]),
                            data.size() - totalSent,
                            0);

        if (sent <= 0)
        {
            std::cerr << "Send failed: " << std::strerror(errno) << std::endl;
            return false;
        }

        totalSent += static_cast<size_t>(sent);
    }

    return true;
}

std::vector<unsigned char> LDAPClient::receivePacket()
{
    std::vector<unsigned char> packet;
    unsigned char buffer[4096];

    ssize_t bytesRead = recv(sockfd, reinterpret_cast<char*>(buffer), sizeof(buffer), 0);
    if (bytesRead <= 0)
    {
        return packet;
    }

    packet.insert(packet.end(), buffer, buffer + bytesRead);
    return packet;
}

void LDAPClient::appendLength(std::vector<unsigned char>& out, unsigned int len)
{
    if (len < 128)
    {
        out.push_back(static_cast<unsigned char>(len));
    }
    else if (len <= 255)
    {
        out.push_back(0x81);
        out.push_back(static_cast<unsigned char>(len));
    }
    else
    {
        out.push_back(0x82);
        out.push_back(static_cast<unsigned char>((len >> 8) & 0xFF));
        out.push_back(static_cast<unsigned char>(len & 0xFF));
    }
}

void LDAPClient::appendInteger(std::vector<unsigned char>& out, int value)
{
    out.push_back(0x02); // INTEGER
    out.push_back(0x01); // length
    out.push_back(static_cast<unsigned char>(value));
}

void LDAPClient::appendOctetString(std::vector<unsigned char>& out, const std::string& value)
{
    out.push_back(0x04); // OCTET STRING
    appendLength(out, static_cast<unsigned int>(value.size()));
    out.insert(out.end(), value.begin(), value.end());
}

void LDAPClient::appendSequence(std::vector<unsigned char>& out, unsigned char tag, const std::vector<unsigned char>& content)
{
    out.push_back(tag);
    appendLength(out, static_cast<unsigned int>(content.size()));
    out.insert(out.end(), content.begin(), content.end());
}

std::vector<unsigned char> LDAPClient::buildAnonymousBindRequest()
{
    std::vector<unsigned char> bindContent;
    std::vector<unsigned char> ldapMessage;

    // version = 3
    appendInteger(bindContent, 3);

    // name = "" (empty DN)
    appendOctetString(bindContent, "");

    // authentication = simple, empty password
    bindContent.push_back(0x80);
    bindContent.push_back(0x00);

    // LDAPMessage
    appendInteger(ldapMessage, nextMessageId++);
    appendSequence(ldapMessage, 0x60, bindContent); // [APPLICATION 0] BindRequest

    std::vector<unsigned char> finalMessage;
    appendSequence(finalMessage, 0x30, ldapMessage); // SEQUENCE

    return finalMessage;
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
    searchContent.push_back(0x0A); // ENUMERATED
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
    searchContent.push_back(0x01); // BOOLEAN
    searchContent.push_back(0x01);
    searchContent.push_back(0x00);

    // filter: equalityMatch (cn=planeName)
    std::vector<unsigned char> equalityMatch;
    appendOctetString(equalityMatch, "cn");
    appendOctetString(equalityMatch, planeName);
    appendSequence(searchContent, 0xA3, equalityMatch); // [3] equalityMatch

    // attributes: request only "description"
    std::vector<unsigned char> attributes;
    appendOctetString(attributes, attributeName);
    appendSequence(searchContent, 0x30, attributes);

    appendInteger(ldapMessage, nextMessageId++);
    appendSequence(ldapMessage, 0x63, searchContent); // [APPLICATION 3] SearchRequest

    std::vector<unsigned char> finalMessage;
    appendSequence(finalMessage, 0x30, ldapMessage);

    return finalMessage;
}

int LDAPClient::findSubsequence(const std::vector<unsigned char>& data, const std::string& text, int start) const
{
    if (text.empty() || data.empty())
    {
        return -1;
    }

    for (size_t i = static_cast<size_t>(start); i + text.size() <= data.size(); ++i)
    {
        bool match = true;
        for (size_t j = 0; j < text.size(); ++j)
        {
            if (data[i + j] != static_cast<unsigned char>(text[j]))
            {
                match = false;
                break;
            }
        }

        if (match)
        {
            return static_cast<int>(i);
        }
    }

    return -1;
}

std::string LDAPClient::parseSearchResultForDescription(const std::vector<unsigned char>& packet)
{
    // Minimal parser for this assignment:
    // Find the bytes for "description", then find the next OCTET STRING after it,
    // and treat that as the value.
    //
    // This is not a complete BER parser, but it is often enough for a simple lab setup.

    int attrPos = findSubsequence(packet, "description", 0);
    if (attrPos < 0)
    {
        return "";
    }

    int valuePos = findSubsequence(packet, "description", attrPos + 1);
    if (valuePos >= 0)
    {
        attrPos = valuePos;
    }

    for (size_t i = static_cast<size_t>(attrPos + 11); i + 2 < packet.size(); ++i)
    {
        if (packet[i] == 0x04) // OCTET STRING
        {
            unsigned int len = packet[i + 1];

            if ((len & 0x80) == 0)
            {
                if (i + 2 + len <= packet.size())
                {
                    return std::string(packet.begin() + i + 2, packet.begin() + i + 2 + len);
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
                        return std::string(packet.begin() + i + 3, packet.begin() + i + 3 + actualLen);
                    }
                }
            }
        }
    }

    return "";
}

bool LDAPClient::anonymousBind()
{
    std::vector<unsigned char> request = buildAnonymousBindRequest();

    if (!sendAll(request))
    {
        return false;
    }

    std::vector<unsigned char> response = receivePacket();
    if (response.empty())
    {
        std::cerr << "Bind response was empty." << std::endl;
        return false;
    }

    return true;
}

std::string LDAPClient::searchPlaneSpeed(const std::string& planeName)
{
    std::vector<unsigned char> request = buildSearchRequest(planeName);

    if (!sendAll(request))
    {
        return "";
    }

    std::vector<unsigned char> combinedResponse;
    while (true)
    {
        std::vector<unsigned char> response = receivePacket();
        if (response.empty())
        {
            break;
        }

        combinedResponse.insert(combinedResponse.end(), response.begin(), response.end());

        // SearchResultDone tag is [APPLICATION 5] = 0x65
        bool foundDone = false;
        for (size_t i = 0; i < response.size(); ++i)
        {
            if (response[i] == 0x65)
            {
                foundDone = true;
                break;
            }
        }

        if (foundDone)
        {
            break;
        }
    }

    return parseSearchResultForDescription(combinedResponse);
}