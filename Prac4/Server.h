#ifndef SERVER_H
#define SERVER_H

#include <atomic>
#include <mutex>
#include <netinet/in.h>
#include <string>
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <ctime>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <thread>
#include <sys/socket.h>
#include <unistd.h>


//Change the includes aftter this
#include "page.h"
#include "database.h"


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

    Page page;
    std::mutex page_mutex;
    Database* db;

};

#endif // SERVER_H