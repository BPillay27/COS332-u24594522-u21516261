#include <iostream>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctime>
#include <stdexcept>
#include <cerrno>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <algorithm>

#include "database.h"

using namespace std;

class serviceSMNTP
{
private:
    int parseCode(const char *buf)
    { // Function to check if therer is a success code else throws an exception
        if (!isdigit((unsigned char)buf[0]) || !isdigit((unsigned char)buf[1]) || !isdigit((unsigned char)buf[2]))
            throw std::runtime_error("invalid SMTP response");
        return (buf[0] - '0') * 100 + (buf[1] - '0') * 10 + (buf[2] - '0');
    }

    bool verifyUser(int sock, const std::string &user, std::string &response)
    {
        std::string msg = "VRFY " + user + "\r\n";
        send(sock, msg.c_str(), msg.length(), 0);

        int code = recvSMTPResponse(sock, response);

        if (code == 250 || code == 251 || code == 252)
        {
            return true;
        }

        return false;
    }

    std::string recvLineCRLF(int sock)
    { // Function to receive response from SMNTP SERVER and checks the size to see if valid response for error handling
        std::string s;
        char c;
        while (true)
        {
            ssize_t r = recv(sock, &c, 1, 0);
            if (r <= 0)
                throw std::runtime_error("recv error/closed");
            s.push_back(c);
            if (s.size() >= 2 && s.substr(s.size() - 2) == "\r\n")
                break;
        }
        return s;
    }

    int recvSMTPResponse(int sock, std::string &out)
    { // Will check if the response is correct for that command
        out = recvLineCRLF(sock);
        if (out.size() < 4)
            throw std::runtime_error("short SMTP response");
        int code = parseCode(out.c_str());
        char sep = out[3];
        if (sep == '-')
        {
            std::string line;
            do
            {
                line = recvLineCRLF(sock);
                out += line;
            } while (!(line.size() >= 4 && parseCode(line.c_str()) == code && line[3] == ' '));
        }
        return code;
    }

    bool isPositiveCompletion(int code)
    {
        return code >= 200 && code < 300;
    }

    bool isPositiveIntermediate(int code)
    {
        return code >= 300 && code < 400;
    }

    bool isTransientNegative(int code)
    {
        return code >= 400 && code < 500;
    }

    bool isPermanentNegative(int code)
    {
        return code >= 500 && code < 600;
    }

    bool codeIn(int code, const std::vector<int> &allowed)
    {
        for (size_t i = 0; i < allowed.size(); ++i)
        {
            if (allowed[i] == code)
                return true;
        }
        return false;
    }

    void expectReply(int code, const std::vector<int> &allowed, const std::string &action, const std::string &response)
    {
        if (codeIn(code, allowed))
            return;

        if (isTransientNegative(code))
        {
            throw std::runtime_error(action + " temporary failure: " + response);
        }

        if (isPermanentNegative(code))
        {
            throw std::runtime_error(action + " permanent failure: " + response);
        }

        throw std::runtime_error(action + " unexpected reply: " + response);
    }

public:
    void sendEmail(const string &user, const vector<string> &events)
    {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in serv_addr;

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(25); // SMTP Port
        inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        {
            cerr << "Connection to SMTP server failed. Is Postfix running?" << endl;
            return;
        }

        std::string response;
        int code = recvSMTPResponse(sock, response);
        expectReply(code, std::vector<int>(1, 220), "server greeting", response);

        string msg = "HELO localhost\r\n"; // The SMTP Handshake
        send(sock, msg.c_str(), msg.length(), 0);
        code = recvSMTPResponse(sock, response);
        expectReply(code, std::vector<int>(1, 250), "HELO", response);

        std::string vrfyResponse = "";
        bool vrfyOk = verifyUser(sock, user, vrfyResponse);

        if (vrfyOk)
        {
            std::cout << "VRFY response: " << vrfyResponse;
        }
        else
        {
            std::cout << "VRFY failed or disabled: " << vrfyResponse;
        }

        msg = "MAIL FROM:<" + user + "@localhost>\r\n";
        send(sock, msg.c_str(), msg.length(), 0);
        code = recvSMTPResponse(sock, response);
        expectReply(code, std::vector<int>(1, 250), "MAIL FROM", response);

        msg = "RCPT TO:<" + user + "@localhost>\r\n";
        send(sock, msg.c_str(), msg.length(), 0);
        code = recvSMTPResponse(sock, response);
        expectReply(code, std::vector<int>(1, 250), "RCPT TO", response);

        msg = "DATA\r\n";
        send(sock, msg.c_str(), msg.length(), 0);
        code = recvSMTPResponse(sock, response);
        expectReply(code, std::vector<int>(1, 354), "DATA", response);

        // Construct the email body
        string content = "Subject: Event Reminder\r\n\r\n";
        content += "Events happening in 6 days:\n";
        for (const auto &e : events)
            content += "- " + e + "\n";
        content += "\r\n.\r\n"; // The period ends the DATA section

        msg = content;
        send(sock, msg.c_str(), msg.length(), 0);
        code = recvSMTPResponse(sock, response);
        expectReply(code, std::vector<int>(1, 250), "email content", response);

        msg = "QUIT\r\n";
        send(sock, msg.c_str(), msg.length(), 0);
        code = recvSMTPResponse(sock, response);
        expectReply(code, std::vector<int>(1, 221), "QUIT", response);

        close(sock);
    }
};

// std::string formattedDate(){
//     std::time_t t = std::time(nullptr);
//     std::tm tm = *std::localtime(&t);
//     std::ostringstream oss;
//     oss << std::put_time(&tm, "%d/%m");
//     return oss.str();
// };

static std::string date_plus_days(int days)
{ // Returns date in "dd/mm" for `days` days from now
    std::time_t t = std::time(nullptr);
    std::tm tm{};
    localtime_r(&t, &tm);
    tm.tm_mday += days;
    std::time_t future = mktime(&tm);
    std::tm tm2{};
    localtime_r(&future, &tm2);
    char buf[6] = {0}; // "dd/mm" + null
    std::strftime(buf, sizeof(buf), "%d/%m", &tm2);
    return std::string(buf);
}

static std::string readUser()
{
    std::ifstream f(".env");
    if (!f.is_open())
    {
        throw std::runtime_error(".env file not found");
    }
    std::string line;
    while (std::getline(f, line))
    {
        // trim leading whitespace
        auto first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos)
            continue;
        if (line[first] == '#')
            continue;
        auto eq = line.find('=', first);
        if (eq == std::string::npos)
            continue;
        std::string key = line.substr(first, eq - first);
        auto key_end = key.find_last_not_of(" \t");
        if (key_end != std::string::npos)
            key = key.substr(0, key_end + 1);
        std::string val = line.substr(eq + 1);
        auto val_first = val.find_first_not_of(" \t");
        if (val_first == std::string::npos)
            val = "";
        else
        {
            val = val.substr(val_first);
            auto val_end = val.find_last_not_of(" \t\r\n");
            if (val_end != std::string::npos)
                val = val.substr(0, val_end + 1);
        }
        if (key == "maiUser" || key == "mailUser")
        {
            if (val.empty())
                throw std::runtime_error("maiUser value empty in .env");
            return val;
        }
    }
    throw std::runtime_error("maiUser not found in .env");
}

int main()
{
    Database *db = new Database("reminder.txt");
    std::string sixDays = date_plus_days(6);
    std::string twoDays = date_plus_days(2);
    serviceSMNTP mailer;
    std::string User = readUser();

    // db->addAppointment(Appointment(sixDays,"Keegan"));
    // db->addAppointment(Appointment(twoDays,"Byron"));

    // db->saveToFile();

    vector<Appointment *> found = db->search(sixDays);

    vector<string> content;
    for (int i = 0; found.size() > (long unsigned)i; i++)
    {
        content.push_back(found.at(i)->toString());
    }
    mailer.sendEmail(User, content);

    delete db;
    return 0;
}
