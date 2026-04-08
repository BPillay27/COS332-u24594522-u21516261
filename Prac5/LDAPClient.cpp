#include "LDAPClient.h"

LDAPClient::LDAPClient(const std::string &host, int port)
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

    if (connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
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

bool LDAPClient::sendAll(const std::vector<unsigned char> &data)
{
    size_t totalSent = 0;

    while (totalSent < data.size())
    {
        ssize_t sent = send(sockfd,
                            reinterpret_cast<const char *>(&data[totalSent]),
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

bool LDAPClient::messageIsSearchResultDone(const std::vector<unsigned char> &packet) const
{
    size_t pos = 0;

    if (pos >= packet.size() || packet[pos] != 0x30)
    {
        return false;
    }

    ++pos;
    size_t msgLen = 0;
    if (!readBERLength(packet, pos, msgLen) || pos + msgLen > packet.size())
    {
        return false;
    }

    if (pos >= packet.size() || packet[pos] != 0x02)
    {
        return false;
    }

    ++pos;
    size_t idLen = 0;
    if (!readBERLength(packet, pos, idLen) || pos + idLen > packet.size())
    {
        return false;
    }

    pos += idLen;

    if (pos >= packet.size())
    {
        return false;
    }

    return packet[pos] == 0x65;
}

std::vector<unsigned char> LDAPClient::receivePacket()
{
    std::vector<unsigned char> packet;
    unsigned char firstByte = 0;

    // Read tag
    ssize_t r = recv(sockfd, reinterpret_cast<char *>(&firstByte), 1, 0);
    if (r <= 0)
    {
        return packet;
    }
    packet.push_back(firstByte);

    // Read first length byte
    r = recv(sockfd, reinterpret_cast<char *>(&firstByte), 1, 0);
    if (r <= 0)
    {
        packet.clear();
        return packet;
    }
    packet.push_back(firstByte);

    size_t contentLength = 0;

    if ((firstByte & 0x80) == 0)
    {
        contentLength = firstByte;
    }
    else
    {
        size_t numLenBytes = firstByte & 0x7F;
        if (numLenBytes == 0)
        {
            packet.clear();
            return packet;
        }

        std::vector<unsigned char> lenBytes(numLenBytes);
        size_t totalRead = 0;

        while (totalRead < numLenBytes)
        {
            r = recv(sockfd,
                     reinterpret_cast<char *>(&lenBytes[totalRead]),
                     numLenBytes - totalRead,
                     0);
            if (r <= 0)
            {
                packet.clear();
                return packet;
            }
            totalRead += static_cast<size_t>(r);
        }

        packet.insert(packet.end(), lenBytes.begin(), lenBytes.end());

        for (size_t i = 0; i < numLenBytes; ++i)
        {
            contentLength = (contentLength << 8) | lenBytes[i];
        }
    }

    std::vector<unsigned char> content(contentLength);
    size_t totalRead = 0;

    while (totalRead < contentLength)
    {
        r = recv(sockfd,
                 reinterpret_cast<char *>(&content[totalRead]),
                 contentLength - totalRead,
                 0);
        if (r <= 0)
        {
            packet.clear();
            return packet;
        }
        totalRead += static_cast<size_t>(r);
    }

    packet.insert(packet.end(), content.begin(), content.end());
    return packet;
}

void LDAPClient::appendLength(std::vector<unsigned char> &out, unsigned int len)
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

void LDAPClient::appendInteger(std::vector<unsigned char> &out, int value)
{
    out.push_back(0x02); // INTEGER
    out.push_back(0x01); // length
    out.push_back(static_cast<unsigned char>(value));
}

void LDAPClient::appendOctetString(std::vector<unsigned char> &out, const std::string &value)
{
    out.push_back(0x04); // OCTET STRING
    appendLength(out, static_cast<unsigned int>(value.size()));
    out.insert(out.end(), value.begin(), value.end());
}

void LDAPClient::appendSequence(std::vector<unsigned char> &out, unsigned char tag, const std::vector<unsigned char> &content)
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

std::vector<unsigned char> LDAPClient::buildSearchRequest(const std::string &planeName)
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

int LDAPClient::findSubsequence(const std::vector<unsigned char> &data, const std::string &text, int start) const
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

std::string LDAPClient::parseSearchResultForDescription(const std::vector<unsigned char> &packet)
{

    std::vector<PlaneInfo> planes;
    if (!extractPlanesFromPacket(packet, planes))
    {
        return "";
    }

    for (size_t i = 0; i < planes.size(); ++i)
    {
        if (!planes[i].speed.empty())
        {
            return planes[i].speed;
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

    return responseIndicatesSuccess(response);
}

std::string LDAPClient::searchPlaneSpeed(const std::string &planeName)
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
        bool foundDone = messageIsSearchResultDone(response);

        if (foundDone)
        {
            break;
        }
    }

    return parseSearchResultForDescription(combinedResponse);
}

bool LDAPClient::responseIndicatesSuccess(const std::vector<unsigned char> &response)
{
    for (size_t i = 0; i + 2 < response.size(); ++i)
    {
        if (response[i] == 0x0A &&
            response[i + 1] == 0x01 &&
            response[i + 2] == 0x00)
        {
            return true;
        }
    }

    return false;
}

bool LDAPClient::simpleBind(const std::string &dn, const std::string &password)
{
    std::vector<unsigned char> bindContent;
    std::vector<unsigned char> ldapMessage;
    std::vector<unsigned char> finalMessage;

    // version = 3
    appendInteger(bindContent, 3);

    // bind DN
    appendOctetString(bindContent, dn);

    // simple authentication choice [0]
    bindContent.push_back(0x80);
    appendLength(bindContent, static_cast<unsigned int>(password.size()));
    bindContent.insert(bindContent.end(), password.begin(), password.end());

    // LDAPMessage
    appendInteger(ldapMessage, nextMessageId++);
    appendSequence(ldapMessage, 0x60, bindContent); // BindRequest

    appendSequence(finalMessage, 0x30, ldapMessage);

    if (!sendAll(finalMessage))
    {
        return false;
    }

    std::vector<unsigned char> response = receivePacket();
    if (response.empty())
    {
        std::cerr << "Bind response was empty." << std::endl;
        return false;
    }

    return responseIndicatesSuccess(response);
}

bool LDAPClient::addPlane(const std::string &planeName, const std::string &speed)
{
    const std::string dn = "cn=" + planeName + ",ou=Planes,dc=assets,dc=local";

    std::vector<unsigned char> addContent;
    std::vector<unsigned char> attributeList;
    std::vector<unsigned char> ldapMessage;
    std::vector<unsigned char> finalMessage;

    // entry DN
    appendOctetString(addContent, dn);

    {
        std::vector<unsigned char> partialAttribute;
        std::vector<unsigned char> setContent;

        // attribute type
        appendOctetString(partialAttribute, "objectClass");

        // values inside SET
        appendOctetString(setContent, "top");
        appendOctetString(setContent, "device");

        // wrap values in SET (0x31)
        appendSequence(partialAttribute, 0x31, setContent);

        // wrap into attribute list
        appendSequence(attributeList, 0x30, partialAttribute);
    }

    {
        std::vector<unsigned char> partialAttribute;
        std::vector<unsigned char> setContent;

        appendOctetString(partialAttribute, "cn");
        appendOctetString(setContent, planeName);
        appendSequence(partialAttribute, 0x31, setContent);

        appendSequence(attributeList, 0x30, partialAttribute);
    }

    {
        std::vector<unsigned char> partialAttribute;
        std::vector<unsigned char> setContent;

        appendOctetString(partialAttribute, "description");
        appendOctetString(setContent, speed);
        appendSequence(partialAttribute, 0x31, setContent);

        appendSequence(attributeList, 0x30, partialAttribute);
    }

    // attributes list
    appendSequence(addContent, 0x30, attributeList);

    // LDAPMessage
    appendInteger(ldapMessage, nextMessageId++);
    appendSequence(ldapMessage, 0x68, addContent); // AddRequest [APPLICATION 8]

    appendSequence(finalMessage, 0x30, ldapMessage);

    if (!sendAll(finalMessage))
    {
        return false;
    }

    std::vector<unsigned char> response = receivePacket();
    if (response.empty())
    {
        std::cerr << "Add response was empty." << std::endl;
        return false;
    }

    return responseIndicatesSuccess(response);
}

bool LDAPClient::deletePlane(const std::string &planeName)
{
    const std::string dn = "cn=" + planeName + ",ou=Planes,dc=assets,dc=local";

    std::vector<unsigned char> ldapMessage;
    std::vector<unsigned char> finalMessage;

    appendInteger(ldapMessage, nextMessageId++);

    // DelRequest is [APPLICATION 10], primitive tag 0x4A
    ldapMessage.push_back(0x4A);
    appendLength(ldapMessage, static_cast<unsigned int>(dn.size()));
    ldapMessage.insert(ldapMessage.end(), dn.begin(), dn.end());

    appendSequence(finalMessage, 0x30, ldapMessage);

    if (!sendAll(finalMessage))
    {
        return false;
    }

    std::vector<unsigned char> response = receivePacket();
    if (response.empty())
    {
        std::cerr << "Delete response was empty." << std::endl;
        return false;
    }

    return responseIndicatesSuccess(response);
}

bool LDAPClient::readBERLength(const std::vector<unsigned char> &data, size_t &pos, size_t &len) const
{
    if (pos >= data.size())
    {
        return false;
    }

    unsigned char first = data[pos++];

    if ((first & 0x80) == 0)
    {
        len = first;
        return true;
    }

    size_t numBytes = first & 0x7F;
    if (numBytes == 0 || numBytes > sizeof(size_t) || pos + numBytes > data.size())
    {
        return false;
    }

    len = 0;
    for (size_t i = 0; i < numBytes; ++i)
    {
        len = (len << 8) | data[pos++];
    }

    return true;
}

bool LDAPClient::extractPlanesFromPacket(const std::vector<unsigned char> &packet, std::vector<PlaneInfo> &planes) const
{
    size_t pos = 0;

    while (pos < packet.size())
    {
        if (packet[pos] != 0x30) // LDAPMessage SEQUENCE
        {
            ++pos;
            continue;
        }

        size_t msgPos = pos + 1;
        size_t msgLen = 0;
        if (!readBERLength(packet, msgPos, msgLen) || msgPos + msgLen > packet.size())
        {
            return false;
        }

        size_t msgEnd = msgPos + msgLen;

        // messageID
        if (msgPos >= msgEnd || packet[msgPos] != 0x02)
        {
            pos = msgEnd;
            continue;
        }

        ++msgPos;
        size_t idLen = 0;
        if (!readBERLength(packet, msgPos, idLen) || msgPos + idLen > msgEnd)
        {
            return false;
        }
        msgPos += idLen;

        if (msgPos >= msgEnd)
        {
            pos = msgEnd;
            continue;
        }

        // Only care about SearchResultEntry
        if (packet[msgPos] != 0x64)
        {
            pos = msgEnd;
            continue;
        }

        ++msgPos;
        size_t entryLen = 0;
        if (!readBERLength(packet, msgPos, entryLen) || msgPos + entryLen > msgEnd)
        {
            return false;
        }

        size_t entryEnd = msgPos + entryLen;

        // objectName (DN)
        if (msgPos >= entryEnd || packet[msgPos] != 0x04)
        {
            pos = msgEnd;
            continue;
        }

        ++msgPos;
        size_t dnLen = 0;
        if (!readBERLength(packet, msgPos, dnLen) || msgPos + dnLen > entryEnd)
        {
            return false;
        }
        msgPos += dnLen;

        // attributes sequence
        if (msgPos >= entryEnd || packet[msgPos] != 0x30)
        {
            pos = msgEnd;
            continue;
        }

        ++msgPos;
        size_t attrsLen = 0;
        if (!readBERLength(packet, msgPos, attrsLen) || msgPos + attrsLen > entryEnd)
        {
            return false;
        }

        size_t attrsEnd = msgPos + attrsLen;

        PlaneInfo plane;
        bool hasName = false;
        bool hasSpeed = false;

        while (msgPos < attrsEnd)
        {
            if (packet[msgPos] != 0x30) // PartialAttribute SEQUENCE
            {
                break;
            }

            ++msgPos;
            size_t paLen = 0;
            if (!readBERLength(packet, msgPos, paLen) || msgPos + paLen > attrsEnd)
            {
                return false;
            }

            size_t paEnd = msgPos + paLen;

            // attribute type
            if (msgPos >= paEnd || packet[msgPos] != 0x04)
            {
                msgPos = paEnd;
                continue;
            }

            ++msgPos;
            size_t typeLen = 0;
            if (!readBERLength(packet, msgPos, typeLen) || msgPos + typeLen > paEnd)
            {
                return false;
            }

            std::string attrType(packet.begin() + msgPos, packet.begin() + msgPos + typeLen);
            msgPos += typeLen;

            // vals SET
            if (msgPos >= paEnd || packet[msgPos] != 0x31)
            {
                msgPos = paEnd;
                continue;
            }

            ++msgPos;
            size_t setLen = 0;
            if (!readBERLength(packet, msgPos, setLen) || msgPos + setLen > paEnd)
            {
                return false;
            }

            size_t setEnd = msgPos + setLen;

            while (msgPos < setEnd)
            {
                if (packet[msgPos] != 0x04)
                {
                    break;
                }

                ++msgPos;
                size_t valueLen = 0;
                if (!readBERLength(packet, msgPos, valueLen) || msgPos + valueLen > setEnd)
                {
                    return false;
                }

                std::string value(packet.begin() + msgPos, packet.begin() + msgPos + valueLen);
                msgPos += valueLen;

                if (attrType == "cn")
                {
                    plane.name = value;
                    hasName = true;
                }
                else if (attrType == "description")
                {
                    plane.speed = value;
                    hasSpeed = true;
                }
            }

            msgPos = paEnd;
        }

        if (hasName || hasSpeed)
        {
            planes.push_back(plane);
        }

        pos = msgEnd;
    }

    return true;
}

std::vector<PlaneInfo> LDAPClient::parseAllPlanes(const std::vector<unsigned char> &packet)
{
    std::vector<PlaneInfo> planes;
    extractPlanesFromPacket(packet, planes);
    return planes;
}

std::vector<PlaneInfo> LDAPClient::getAllPlanes()
{
    const std::string baseDN = "ou=Planes,dc=assets,dc=local";

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

    // filter: (objectClass=*)
    searchContent.push_back(0x87); // present filter
    appendLength(searchContent, 11);
    searchContent.insert(searchContent.end(), "objectClass", "objectClass" + 11);

    // request attributes: cn and description
    std::vector<unsigned char> attributes;
    appendOctetString(attributes, "cn");
    appendOctetString(attributes, "description");
    appendSequence(searchContent, 0x30, attributes);

    appendInteger(ldapMessage, nextMessageId++);
    appendSequence(ldapMessage, 0x63, searchContent); // SearchRequest

    std::vector<unsigned char> finalMessage;
    appendSequence(finalMessage, 0x30, ldapMessage);

    if (!sendAll(finalMessage))
    {
        return std::vector<PlaneInfo>();
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

        bool foundDone = messageIsSearchResultDone(response);

        if (foundDone)
        {
            break;
        }
    }

    return parseAllPlanes(combinedResponse);
}