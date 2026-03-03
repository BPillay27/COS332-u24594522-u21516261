#ifndef SERVER_H
#define SERVER_H

#include <atomic>
#include <netinet/in.h>
#include <string>

class Server {
public:
    Server(int port);
    void run();
    void stop();

private:
    std::atomic<bool> running{false};
    int server_no = -1;
    struct sockaddr_in address;
    void handle_client(int client_fd);
    void send_response(int client_fd, const std::string& response);
    std::string read_request(int client_fd);
    std::string process_request(const std::string& request);
};

#endif // SERVER_H