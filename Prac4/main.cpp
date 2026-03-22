#include "Server.h"
#include <iostream>
#include <thread>
#include <unistd.h> // For close()
#include <sys/socket.h>
#include <netinet/in.h>

int main(int argc, char **argv) {
    int port = 8080;
    if (argc > 1) {
        port = std::stoi(argv[1]);
    }
    Server s(port);

    std::thread shutdown_listener([&s, &port]() {
        std::string command;
        while (std::getline(std::cin, command)) {
            if (command == "stop") {
                std::cout << "Stopping server from terminal command...\n";
                s.stop();
                // Self-connect to unblock accept()
                int sock = socket(AF_INET, SOCK_STREAM, 0);
                if (sock >= 0) {
                    sockaddr_in addr{};
                    addr.sin_family = AF_INET;
                    addr.sin_port = htons(static_cast<uint16_t>(port));
                    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
                    connect(sock, (sockaddr*)&addr, sizeof(addr));
                    close(sock);
                }
                break;
            }
        }
    });
    std::cout << "Type 'stop' in this terminal to shut down the server.\n";
    shutdown_listener.detach();
    s.run();
    return 0;
}