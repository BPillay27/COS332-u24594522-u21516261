#include "Server.h"
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


Server::Server(int port) {
    std::memset(&address, 0, sizeof(address));
    if ((server_no = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
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
        if (listen(server_no, SOMAXCONN) < 0){
        std::cerr << "Listen failed: " << std::strerror(errno) << "\n";
        std::exit(EXIT_FAILURE);
    }
    std::cout << "Server listening on port " << ntohs(address.sin_port) << std::endl;
}

void Server::run() {
    running = true;
    while (running) {
        socklen_t addrlen = sizeof(address);
        int client_fd = accept(server_no, (struct sockaddr *)&address, &addrlen);
        if (client_fd < 0) {
            if (!running || errno == EINTR || errno == EBADF)
                break;
            std::cerr << "Accept failed: " << std::strerror(errno) << "\n";
            continue;
        }
        std::thread(&Server::handle_client, this, client_fd).detach();
    }
    std::cout << "Server shutting down...\n";
}

void Server::stop() {
    running = false;
    close(server_no);
}

void Server::handle_client(int client_fd) {
    std::string request = read_request(client_fd);
    std::string response = process_request(request);
    send_response(client_fd, response); 
    close(client_fd);
}

void Server::send_response(int client_fd, const std::string& response) {
    send(client_fd, response.c_str(), response.size(), 0);
}

std::string Server::read_request(int client_fd) {
    char buffer[1024] = {0};
    ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        return std::string(buffer);
    }
    return "";
}

std::string Server::process_request(const std::string& request) {
    //Change this depending on the server's functionality. For now, it just checks if the request starts with "GET " and responds with a simple message.    
    if (request.find("GET ") == 0) {
        std::string html = "<html><head><title>Welcome</title></head><body><h1>Hello, world!</h1><p>This is a simple C++ HTTP server.</p></body></html>";
        std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " + std::to_string(html.size()) + "\r\n\r\n" + html;
        return response;
    } else {
        std::string html = "<html><head><title>Error</title></head><body><h1>400 Bad Request</h1></body></html>";
        std::string response = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html\r\nContent-Length: " + std::to_string(html.size()) + "\r\n\r\n" + html;
        return response;
    }
}





