#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <ctime>
#include <iostream>
#include <netinet/in.h> //needed for sockaddr_in and to convert the port number to network byte order
#include <string>
#include <thread>
#include <atomic>
#include <set>
#include <mutex>
#include <sys/socket.h>
#include <unistd.h>
#include <regex>

class Server
{
public:
    Server(int port)
    {
        std::memset(&address, 0, sizeof(address)); // Zeroing out memory to clear garabge

        if ((server_no = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            std::cerr << "Socket creation failed: " << std::strerror(errno) << "\n";
            std::exit(EXIT_FAILURE);
        }

        int opt = 1;
        setsockopt(server_no, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(static_cast<uint16_t>(port));

        // Bind
        if (bind(server_no, (struct sockaddr *)&address, sizeof(address)) < 0)
        {
            std::cerr << "Bind failed: " << std::strerror(errno) << "\n";
            std::exit(EXIT_FAILURE);
        }

        if (listen(server_no, SOMAXCONN) < 0)
        {
            std::cerr << "Listen failed: " << std::strerror(errno) << "\n";
            std::exit(EXIT_FAILURE);
        }

        std::cout << "Server listening on port " << port << std::endl;
    }

private:
    std::atomic<bool> running{false};
    int server_no = -1;
    std::set<int> client;
    std::mutex clients_mutex;
    struct sockaddr_in address;
};

int main()
{
}