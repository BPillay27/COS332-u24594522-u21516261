
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
string trim(const string& s) {
    size_t first = s.find_first_not_of(" \t\r\n\"");
    if (string::npos == first) return "";
    size_t last = s.find_last_not_of(" \t\r\n\"");
    return s.substr(first, (last - first + 1));
}

// --- SMTP Service Class ---
class serviceSMNTP {
private:
    int recvResponse(int sock) {
        char buf[1024] = {0};
        recv(sock, buf, 1024, 0);
        return atoi(buf);
    }

public:
    void sendMail(const string& from, const string& to, const string& subj, const string& body) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(25);
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

        if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) return;

        recvResponse(sock); 
        auto cmd = [&](string c) {
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

// --- POP3 Service Class ---
class servicePOP3 {
private:
    string getHeader(const string& data, string key) {
        // Search for the key (e.g., "Subject:") case-insensitively if possible, 
        // but at least handle the basic "Key: "
        size_t pos = data.find(key + ": ");
        if (pos == string::npos) return "";

        size_t start = pos + key.length() + 2;
        size_t end = data.find("\r\n", start);
        
        if (end == string::npos) return ""; 
        
        return trim(data.substr(start, end - start));
    }

public:
    void runVacationResponder(const string& user, const string& pass, serviceSMNTP& mailer) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(110);
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

        if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) return;

        char buf[4096];
        recv(sock, buf, 4096, 0);
        
        send(sock, ("USER " + user + "\r\n").c_str(), user.length() + 7, 0); 
        recv(sock, buf, 4096, 0);
        send(sock, ("PASS " + pass + "\r\n").c_str(), pass.length() + 7, 0); 
        recv(sock, buf, 4096, 0);

        send(sock, "STAT\r\n", 6, 0); 
        memset(buf, 0, 4096);
        recv(sock, buf, 4096, 0);
        
        int count = 0; 
        sscanf(buf, "+OK %d", &count);
        
        // Debugging: See if we actually see messages
        if(count > 0) cout << "Processing " << count << " messages..." << endl;

        static set<string> replied; 

        for (int i = 1; i <= count; ++i) {
            string cmd = "TOP " + to_string(i) + " 0\r\n";
            send(sock, cmd.c_str(), cmd.length(), 0);
            
            // Give the server a tiny bit of time to respond or use a better recv loop
            memset(buf, 0, 4096);
            int bytes = recv(sock, buf, 4096, 0);
            string headers(buf, bytes);

            string from = getHeader(headers, "From");
            string subj = getHeader(headers, "Subject");
            
            // Robust Email Extraction
            size_t s = from.find("<"), e = from.find(">");
            if (s != string::npos && e != string::npos) {
                from = from.substr(s + 1, e - s - 1);
            } else {
                from = trim(from); // Just use the raw string if no brackets
            }

            // Check if it's the right subject AND we haven't replied yet
            if (subj.find("prac7") != string::npos) {
                if (replied.find(from) == replied.end()) {
                    // Send to the EXACT address we found
                    mailer.sendMail(user, from, "Re: prac7 (Vacation)", "I am away on holiday!");
                    replied.insert(from);
                    cout << "SUCCESS: Vacation reply sent to: " << from << endl;
                } else {
                    cout << "Already replied to " << from << " this session." << endl;
                }
            }
        }
        send(sock, "QUIT\r\n", 6, 0);
        close(sock);
    }
};

// --- Utilities ---
string readEnv(string targetKey) {
    ifstream f(".env");
    string line;
    while (getline(f, line)) {
        size_t eqPos = line.find('=');
        if (eqPos != string::npos) {
            string key = trim(line.substr(0, eqPos));
            if (key == targetKey) {
                return trim(line.substr(eqPos + 1));
            }
        }
    }
    return "";
}
bool keepRunning = true;

void signalHandler(int signum) {
    cout << "\nInterrupt signal (" << signum << ") received. Cleaning up...\n";
    keepRunning = false; // This breaks the while(true) loop gracefully
}

int main() {
    serviceSMNTP mailer;
    servicePOP3 receiver;
    signal(SIGINT, signalHandler);
    
    
    string user = readEnv("mailUser");
    string pass = readEnv("mailPass");//This is needed
    if (user.empty()) {
        cerr << "Error: User not found in .env" << endl;
        return 1;
    }

    cout << "Vacation Bot active for: " << user << endl;
    cout << "Press Ctrl+C to stop the service." << endl;

    // --- The Active Loop ---
    while (keepRunning) {
        try {
            cout << "[" << time(0) << "] Checking for new 'prac7' messages..." << endl;
            receiver.runVacationResponder(user, pass, mailer);
        } catch (const exception& e) {
            cerr << "Connection error: " << e.what() << endl;

        }

        cout << "Waiting 30 seconds until next check (Press Ctrl+C to stop)..." << endl;
        for (int i = 0; i < 30; ++i) {
            if (!keepRunning) break;    // Exit the wait loop immediately if Ctrl+C was hit
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    cout << "Program exited safely. All connections closed." << endl;
    return 0;

}