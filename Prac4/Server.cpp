#include "Server.h"

Server::Server(int port) {
    db=new Database("database.txt");
    page = Page();
    //Remove above for other server.
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
    delete db;
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
    std::string request;
    char buffer[1024];
    // Read headers (until CRLF CRLF)
    while (request.find("\r\n\r\n") == std::string::npos) {
        ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
        if (bytes_read <= 0) break;
        buffer[bytes_read] = '\0';
        request += buffer;
    }

    // If there's a Content-Length header, read the remaining body bytes
    size_t headers_end = request.find("\r\n\r\n");
    if (headers_end != std::string::npos) {
        size_t cl_pos = request.find("Content-Length:");
        if (cl_pos == std::string::npos) cl_pos = request.find("content-length:");
        if (cl_pos != std::string::npos) {
            size_t val_start = request.find_first_not_of(' ', cl_pos + std::string("Content-Length:").size());
            if (val_start != std::string::npos) {
                size_t val_end = request.find("\r\n", val_start);
                if (val_end != std::string::npos && val_end > val_start) {
                    std::string val = request.substr(val_start, val_end - val_start);
                    try {
                        long content_length = std::stol(val);
                        if (content_length > 0) {
                            
                            size_t already = request.size() - (headers_end + 4);
                            long remaining = content_length - static_cast<long>(already);
                            while (remaining > 0) {
                                ssize_t bytes_read = read(client_fd, buffer, (size_t)std::min<long>(sizeof(buffer) - 1, remaining));
                                if (bytes_read <= 0) break;
                                buffer[bytes_read] = '\0';
                                request.append(buffer, (size_t)bytes_read);
                                remaining -= bytes_read;
                            }
                        }
                    } catch (...) {
                        // Ignoring the parsing errors
                    }
                }
            }
        }
    }

    return request;
}

static std::string httpDate() {
    std::time_t now = std::time(nullptr);
    std::tm *gmt = std::gmtime(&now);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", gmt);
    return std::string(buf);
}

// Inject Date header after the status line of any response
static std::string withDate(const std::string& response) {
    size_t pos = response.find("\r\n");
    if (pos == std::string::npos) return response;
    std::string r = response;
    r.insert(pos + 2, "Date: " + httpDate() + "\r\n");
    return r;
}

static const std::string AUTH_TOKEN = "Admin";

std::string Server::process_request(const std::string& request) {
    //Change this depending on the server's functionality. For now, it just checks if the request starts with "GET " and responds with a simple message. 
    std::cout<< "Received request:\n" << request << "\n" << std::endl;
    //For testing
    std::lock_guard<std::mutex> lock(page_mutex);
    std::string response="";
    if (request.find("GET ") == 0) {
        if(request.find("/searchAppointments??keyword=<") != std::string::npos){
            size_t pos = request.find("<") + 1;
            size_t pos2 = request.find(">")-1;
            std::string keyword = request.substr(pos, pos2-pos);
           
            std::vector<Appointment*> array=db->search(keyword);
            //page.clearPage();
            page.updateDays(array);
            page.generateGeneric();

            response = "HTTP/1.1 200 Found\r\nLocation: /\r\nContent-Length: "+std::to_string(page.getHTML().size()) + "\r\n\r\n"+page.getHTML();
        }else if(request.find("/favicon.ico HTTP/1.1") != std::string::npos){
            //page.clearPage();
            page.updateDays(db->getDays());
            page.generateGeneric();
            response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " + std::to_string(page.getHTML().size()) + "\r\n\r\n" + page.getHTML();
            std::cout<< "Favicon request received, responding with main page.\nThe main page:\n";
            //std::cout<< page.getHTML();
        }else if(request.find("GET / HTTP/1.1") == 0 || request.find("GET / HTTP/1.0") == 0){
            //page.clearPage();
            page.updateDays(db->getDays());
            page.generateGeneric();
            response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " + std::to_string(page.getHTML().size()) + "\r\n\r\n" + page.getHTML();
        }else{
            page.clearPage();
            page.appendHTML("<p>You think I would allow you to do what ever you want, keep dreaming </p>");
            response = "HTTP/1.1 405 Method Not Allowed\r\nAllow: GET\r\nContent-Type: text/html\r\nContent-Length: " + std::to_string(page.getHTML().size()) + "\r\n\r\n" + page.getHTML();
        }
    } else if (request.find("POST ") == 0) {
        if(request.find("/deleteAppointment") != std::string::npos){
            size_t pos = request.find("<") + 1;
            size_t pos2 = request.find(">")-1;
            std::string keyword = request.substr(pos, pos2);
           
            std::vector<Appointment*> array=db->search(keyword);

            //std::cout<<db->printAll();
            page.clearPage();
            page.updateDays(array);
            page.generateGeneric();
            response = "HTTP/1.1 200 Found\r\nLocation: /\r\nContent-Length: "+std::to_string(page.getHTML().size()) + "\r\n\r\n"+page.getHTML();
        }else if(request.find("/favicon.ico HTTP/1.1") != std::string::npos){
           // std::cout<<db->printAll();
            page.clearPage();
            page.updateDays(db->getDays());
            page.generateGeneric();
            response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " + std::to_string(page.getHTML().size()) + "\r\n\r\n" + page.getHTML();
            std::cout<< "Favicon request received, responding with main page.\n";
        }else if(request.find("GET / HTTP/1.1") == 0 || request.find("GET / HTTP/1.0") == 0){
            page.clearPage();
            page.updateDays(db->getDays());
            page.generateGeneric();
            response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " + std::to_string(page.getHTML().size()) + "\r\n\r\n" + page.getHTML();
        }else{
            page.clearPage();
            page.appendHTML("<p>You think I would allow you to do what ever you want, keep dreaming </p>");
            response = "HTTP/1.1 405 Method Not Allowed\r\nAllow: GET\r\nContent-Type: text/html\r\nContent-Length: " + std::to_string(page.getHTML().size()) + "\r\n\r\n" + page.getHTML();
        }
    }else{
        page.clearPage();
        page.appendHTML("<p> Bad Request: This was not generated by our page</p>");
        response = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html\r\nContent-Length: " + std::to_string(page.getHTML().size()) + "\r\n\r\n" + page.getHTML();
        
    }

    return withDate(response);
}





