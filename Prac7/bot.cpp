
#include <iostream>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctime>
#include <stdexcept>
#include <cstring>
#include <csignal>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <set>
#include <chrono>
#include <thread>

using namespace std;

// static set<string> replied;
set<string> loadHandledUIDLs(const string &filename);
void appendHandledUIDL(const string &filename, const string &uidl);

string trim(const string &s)
{
    size_t first = s.find_first_not_of(" \t\r\n\"");
    if (string::npos == first)
        return "";
    size_t last = s.find_last_not_of(" \t\r\n\"");
    return s.substr(first, (last - first + 1));
}

// SMTP Service Class
class serviceSMNTP
{
private:
    int recvResponse(int sock)
    {
        char buf[1024] = {0};
        recv(sock, buf, 1024, 0);
        return atoi(buf);
    }

public:
    void sendMail(const string &from, const string &to, const string &subj, const string &body)
    {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(25);
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

        if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
            return;

        recvResponse(sock);
        auto cmd = [&](string c)
        {
            send(sock, c.c_str(), c.length(), 0);
            recvResponse(sock);
        };

        cmd("HELO localhost\r\n");
        cmd("MAIL FROM:<" + from + "@localhost>\r\n");
        cmd("RCPT TO:<" + to + ">\r\n");
        cmd("DATA\r\n");

        string fullMsg = "Subject: " + subj + "\r\n\r\n" + body + "\r\n.\r\n";
        send(sock, fullMsg.c_str(), fullMsg.length(), 0);
        recvResponse(sock);

        cmd("QUIT\r\n");
        close(sock);
    }
};

// POP3 Service Class
class servicePOP3
{
private:
    bool deleteMessage(int sock, int msgNum)
    {
        string cmd = "DELE " + to_string(msgNum) + "\r\n";
        send(sock, cmd.c_str(), cmd.length(), 0);

        string response = recvLine(sock);
        return response.find("+OK") == 0;
    }

    bool resetDeletions(int sock)
    {
        send(sock, "RSET\r\n", 6, 0);

        string response = recvLine(sock);
        return response.find("+OK") == 0;
    }

    string getHeader(const string &data, string key)
    {
        size_t pos = data.find(key + ": ");
        if (pos == string::npos)
            return "";

        size_t start = pos + key.length() + 2;
        size_t end = data.find("\r\n", start);

        if (end == string::npos)
            return "";

        return trim(data.substr(start, end - start));
    }

    time_t parseEmailDate(string dateStr)
    {
        if (dateStr.empty())
            return 0;

        struct tm tm = {0};
        size_t firstDigit = dateStr.find_first_of("0123456789");
        if (firstDigit == string::npos)
            return 0;

        string cleanDate = dateStr.substr(firstDigit);

        // Format: 19 Apr 2026 17:00:12
        if (strptime(cleanDate.c_str(), "%d %b %Y %H:%M:%S", &tm) != NULL)
        {
            return mktime(&tm);
        }
        return 0;
    }

    string recvLine(int sock)
    {
        string response;
        char ch;

        while (true)
        {
            int bytes = recv(sock, &ch, 1, 0);
            if (bytes <= 0)
                break;

            response += ch;

            if (response.size() >= 2 &&
                response[response.size() - 2] == '\r' &&
                response[response.size() - 1] == '\n')
            {
                break;
            }
        }

        return response;
    }

    string recvMultiLine(int sock)
    {
        string response;
        char buf[1024];

        while (true)
        {
            memset(buf, 0, sizeof(buf));
            int bytes = recv(sock, buf, sizeof(buf) - 1, 0);
            if (bytes <= 0)
                break;

            response.append(buf, bytes);

            if (response.find("\r\n.\r\n") != string::npos)
            {
                break;
            }
        }

        return response;
    }

    string getUIDL(int sock, int msgNum)
    {
        string cmd = "UIDL " + to_string(msgNum) + "\r\n";
        send(sock, cmd.c_str(), cmd.length(), 0);

        string response = recvLine(sock);

        if (response.find("+OK") != 0)
        {
            return "";
        }

        istringstream iss(response);
        string ok, num, uidl;
        iss >> ok >> num >> uidl;

        return trim(uidl);
    }

public:
    void runVacationResponder(const string &user, const string &pass, serviceSMNTP &mailer, time_t vacationStart, set<string> &handledUIDLs, const string &uidlFile)
    {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(110);
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

        if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
            return;

        string response = recvLine(sock); // greeting

        send(sock, ("USER " + user + "\r\n").c_str(), user.length() + 7, 0);
        response = recvLine(sock);

        send(sock, ("PASS " + pass + "\r\n").c_str(), pass.length() + 7, 0);
        response = recvLine(sock);

        send(sock, "STAT\r\n", 6, 0);
        response = recvLine(sock);

        int count = 0;
        sscanf(response.c_str(), "+OK %d", &count);

        if (count > 0)
            cout << "Processing " << count << " messages..." << endl;

        for (int i = 1; i <= count; ++i)
        {
            string uidl = getUIDL(sock, i);

            if (uidl.empty())
            {
                cout << "Could not get UIDL for message " << i << endl;
                continue;
            }

            if (handledUIDLs.find(uidl) != handledUIDLs.end())
            {
                cout << "Message " << i << " already handled. Skipping." << endl;
                continue;
            }

            string cmd = "TOP " + to_string(i) + " 0\r\n";
            send(sock, cmd.c_str(), cmd.length(), 0);

            string headers = recvMultiLine(sock);

            string from = getHeader(headers, "From");
            string subj = getHeader(headers, "Subject");
            string dateStr = getHeader(headers, "Date");
            time_t emailTime = parseEmailDate(dateStr);

            size_t s = from.find("<"), e = from.find(">");
            if (s != string::npos && e != string::npos)
            {
                from = from.substr(s + 1, e - s - 1);
            }
            else
            {
                from = trim(from);
            }

            if (subj.find("prac7") != string::npos)
            {
                if (emailTime >= vacationStart)
                {
                    mailer.sendMail(user, from, "Re: prac7 (Vacation)", "I am away on holiday!");
                    cout << "SUCCESS: Vacation reply sent to: " << from << endl;
                }
                else
                {
                    cout << "Old email, not responding" << endl;
                }

                handledUIDLs.insert(uidl);
                appendHandledUIDL(uidlFile, uidl);
            }
        }
        send(sock, "QUIT\r\n", 6, 0);
        close(sock);
    }

    void demoDeleteAndReset(const string &user, const string &pass)
    {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(110);
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

        if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        {
            perror("POP3 connect failed");
            return;
        }

        string response = recvLine(sock); // greeting

        send(sock, ("USER " + user + "\r\n").c_str(), user.length() + 7, 0);
        response = recvLine(sock);
        if (response.find("+OK") != 0)
        {
            cout << "USER failed: " << response;
            close(sock);
            return;
        }

        send(sock, ("PASS " + pass + "\r\n").c_str(), pass.length() + 7, 0);
        response = recvLine(sock);
        if (response.find("+OK") != 0)
        {
            cout << "PASS failed: " << response;
            close(sock);
            return;
        }

        send(sock, "STAT\r\n", 6, 0);
        response = recvLine(sock);

        int count = 0;
        sscanf(response.c_str(), "+OK %d", &count);

        if (count <= 0)
        {
            cout << "No messages available to mark for deletion." << endl;
            send(sock, "QUIT\r\n", 6, 0);
            close(sock);
            return;
        }

        cout << "Most recent message is: " << count << endl;

        if (deleteMessage(sock, count))
        {
            cout << "Message " << count << " marked for deletion." << endl;
        }
        else
        {
            cout << "Failed to mark message " << count << " for deletion." << endl;
            send(sock, "QUIT\r\n", 6, 0);
            close(sock);
            return;
        }

        if (resetDeletions(sock))
        {
            cout << "Deletion marks cleared with RSET." << endl;
        }
        else
        {
            cout << "RSET failed." << endl;
        }

        send(sock, "QUIT\r\n", 6, 0);
        close(sock);
    }
};

// --- Utilities ---
string readEnv(string targetKey)
{
    ifstream f(".env");
    string line;
    while (getline(f, line))
    {
        size_t eqPos = line.find('=');
        if (eqPos != string::npos)
        {
            string key = trim(line.substr(0, eqPos));
            if (key == targetKey)
            {
                return trim(line.substr(eqPos + 1));
            }
        }
    }
    return "";
}

set<string> loadHandledUIDLs(const string &filename)
{
    set<string> ids;
    ifstream in(filename.c_str());
    string line;

    while (getline(in, line))
    {
        line = trim(line);
        if (!line.empty())
        {
            ids.insert(line);
        }
    }

    return ids;
}

void appendHandledUIDL(const string &filename, const string &uidl)
{
    ofstream out(filename.c_str(), ios::app);
    if (out.is_open())
    {
        out << uidl << endl;
    }
}

bool keepRunning = true;

void signalHandler(int signum)
{
    cout << "\nInterrupt signal (" << signum << ") received. Cleaning up...\n";
    keepRunning = false; // This breaks the while(true) loop gracefully
}

int main()
{
    serviceSMNTP mailer;
    servicePOP3 receiver;
    signal(SIGINT, signalHandler);
    time_t vacationStart = time(0); // Start of the holiday

    string user = readEnv("mailUser");
    string pass = readEnv("mailPass"); // This is needed

    string uidlFile = "handled_uidls.txt";
    set<string> handledUIDLs = loadHandledUIDLs(uidlFile);

    if (user.empty())
    {
        cerr << "Error: User not found in .env" << endl;
        return 1;
    }

    cout << "Vacation Bot active for: " << user << endl;
    cout << "Press Ctrl+C to stop the service." << endl;

    // --- The Active Loop ---
    while (keepRunning)
    {
        try
        {
            cout << "[" << time(0) << "] Checking for new 'prac7' messages..." << endl;
            receiver.demoDeleteAndReset(user, pass);
            receiver.runVacationResponder(user, pass, mailer, vacationStart, handledUIDLs, uidlFile);
        }
        catch (const exception &e)
        {
            cerr << "Connection error: " << e.what() << endl;
        }

        cout << "Waiting 30 seconds until next check (Press Ctrl+C to stop)..." << endl;
        for (int i = 0; i < 30; ++i)
        {
            if (!keepRunning)
                break; // Exit the wait loop immediately if Ctrl+C was hit
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    cout << "Program exited safely. All connections closed." << endl;
    return 0;
}