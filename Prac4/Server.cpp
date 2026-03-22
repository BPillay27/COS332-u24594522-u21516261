#include "Server.h"
#include <sys/stat.h>
#include <fstream>

Server::Server(int port) {
    db=new Database("database.txt");
    page = Page();
        // seed RNG for unique filenames
        srand(static_cast<unsigned>(std::time(nullptr)));
    // ensure uploads directory exists
    mkdir("uploads", 0755);
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

std::string Server::url_decode(const std::string &s){
    std::string out; out.reserve(s.size());
    for(size_t i=0;i<s.size();++i){
        if(s[i]=='%'){
            if(i+2<s.size()){
                int v = std::stoi(s.substr(i+1,2),nullptr,16);
                out.push_back((char)v);
                i+=2;
            }
        } else if(s[i]=='+') out.push_back(' ');
        else out.push_back(s[i]);
    }
    return out;
}

std::map<std::string,std::string> Server::parse_urlencoded(const std::string &body){
    std::map<std::string,std::string> m;
    size_t i=0;
    while(i<body.size()){
        size_t amp = body.find('&', i);
        std::string pair = body.substr(i, (amp==std::string::npos? body.size():amp) - i);
        size_t eq = pair.find('=');
        std::string k = url_decode(pair.substr(0, eq));
        std::string v = eq==std::string::npos ? "" : url_decode(pair.substr(eq+1));
        m[k]=v;
        if(amp==std::string::npos) break;
        i = amp + 1;
    }
    return m;
}

std::map<std::string,std::string> Server::parse_multipart(const std::string &body, const std::string &boundary){
    std::map<std::string,std::string> fields;
    std::string delim = "--" + boundary;
    size_t pos = 0;
    while(true){
        size_t start = body.find(delim, pos);
        if(start == std::string::npos) break;
        start += delim.size();
        // check for final boundary
        if(body.substr(start,2) == "--") break;
        // skip CRLF
        if(body.substr(start,2) == "\r\n") start += 2;
        size_t part_end = body.find(delim, start);
        if(part_end == std::string::npos) break;
        std::string part = body.substr(start, part_end - start);
        size_t h_end = part.find("\r\n\r\n");
        if(h_end == std::string::npos) { pos = part_end; continue; }
        std::string ph = part.substr(0, h_end);
        std::string pbody = part.substr(h_end + 4);
        // remove possible trailing CRLF from pbody
        if(pbody.size() >= 2 && pbody.substr(pbody.size()-2) == "\r\n") pbody.erase(pbody.size()-2);

        std::string name;
        std::string filename;
        std::istringstream hs(ph);
        std::string line;
        while(std::getline(hs, line)){
            if(!line.empty() && line.back()=='\r') line.pop_back();
            auto cdpos = line.find("Content-Disposition:");
            if(cdpos != std::string::npos){
                auto npos = line.find("name=", cdpos);
                if(npos != std::string::npos){
                    size_t q1 = line.find('"', npos);
                    size_t q2 = (q1==std::string::npos) ? std::string::npos : line.find('"', q1+1);
                    if(q1!=std::string::npos && q2!=std::string::npos) name = line.substr(q1+1, q2-q1-1);
                }
                auto fpos = line.find("filename=");
                if(fpos != std::string::npos){
                    size_t q1 = line.find('"', fpos);
                    size_t q2 = (q1==std::string::npos) ? std::string::npos : line.find('"', q1+1);
                    if(q1!=std::string::npos && q2!=std::string::npos) filename = line.substr(q1+1, q2-q1-1);
                }
            }
        }

        if(!filename.empty()){
            // treat as file upload: save to uploads/ and store the path
            std::string dir = "uploads";
            // sanitize/derive extension
            std::string ext;
            size_t dot = filename.find_last_of('.');
            if(dot!=std::string::npos) ext = filename.substr(dot);
            // create unique file name
            std::string uniq = std::to_string(std::time(nullptr)) + "_" + std::to_string(rand() % 100000);
            std::string savedPath = dir + "/img_" + uniq + ext;
            std::ofstream ofs(savedPath, std::ios::binary);
            if(ofs){ ofs.write(pbody.data(), pbody.size()); ofs.close(); }
            // store URL path (leading slash for GET request handling)
            fields[name] = std::string("/") + savedPath;
        } else if(!name.empty()){
            fields[name] = pbody;
        }

        pos = part_end;
    }
    return fields;
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
    
    //For testing
    std::lock_guard<std::mutex> lock(page_mutex);
    std::string response="";
    if (request.find("GET ") == 0) {
        // quick static file serving for uploads
        size_t path_start = 4;
        size_t path_end = request.find(' ', path_start);
        std::string reqPath;
        if (path_end != std::string::npos) {
            reqPath = request.substr(path_start, path_end - path_start);
            if (reqPath.rfind("/uploads/", 0) == 0) {
                std::string fsPath = reqPath.substr(1); // strip leading '/'
                std::ifstream ifs(fsPath, std::ios::binary);
                if (ifs) {
                    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
                    std::string contentType = "application/octet-stream";
                    if (reqPath.find(".png") != std::string::npos) contentType = "image/png";
                    else if (reqPath.find(".jpg") != std::string::npos || reqPath.find(".jpeg") != std::string::npos) contentType = "image/jpeg";
                    else if (reqPath.find(".gif") != std::string::npos) contentType = "image/gif";
                    std::string resp = "HTTP/1.1 200 OK\r\nContent-Type: " + contentType + "\r\nContent-Length: " + std::to_string(content.size()) + "\r\n\r\n" + content;
                    return withDate(resp);
                }
            }
        }
        if(reqPath.rfind("/searchAppointments", 0) == 0) {
            // parse query string like /searchAppointments?keyword=Byron
            std::string keyword;
            size_t qpos = reqPath.find('?');
            if (qpos != std::string::npos && qpos + 1 < reqPath.size()) {
                std::string query = reqPath.substr(qpos + 1);
                size_t kpos = query.find("keyword=");
                if (kpos != std::string::npos) {
                    size_t val_start = kpos + 8;
                    size_t amp = query.find('&', val_start);
                    std::string raw = (amp == std::string::npos) ? query.substr(val_start) : query.substr(val_start, amp - val_start);
                    keyword = url_decode(raw);
                }
            }

            std::vector<Appointment*> array = db->search(keyword);
            page.updateDays(array);
            page.generateGeneric();
            response = "HTTP/1.1 200 Found\r\nLocation: /\r\nContent-Length: "+std::to_string(page.getHTML().size()) + "\r\n\r\n"+page.getHTML();
        } else if(request.find("/favicon.ico HTTP/1.1") != std::string::npos){
            //page.clearPage();
            page.updateDays(db->getDays());
            page.generateGeneric();
            response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " + std::to_string(page.getHTML().size()) + "\r\n\r\n" + page.getHTML();
            //std::cout<< "Favicon request received, responding with main page.\nThe main page:\n";
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
            size_t h_end = request.find("\r\n\r\n");
            std::string headers = (h_end==std::string::npos) ? std::string() : request.substr(0, h_end);
            std::string body = (h_end==std::string::npos) ? std::string() : request.substr(h_end+4);
            std::map<std::string,std::string> fields;
            // choose parser based on Content-Type
            if(headers.find("multipart/form-data") != std::string::npos){
                size_t bpos = headers.find("boundary=");
                if(bpos != std::string::npos){
                    std::string boundary = headers.substr(bpos + 9);
                    // trim possible trailing chars
                    auto semi = boundary.find_first_of("\r\n; ");
                    if(semi != std::string::npos) boundary = boundary.substr(0, semi);
                    fields = parse_multipart(body, boundary);
                } else {
                    fields = parse_urlencoded(body);
                }
            } else {
                fields = parse_urlencoded(body);
            }
            std::string date = fields["date"];
            std::string time = fields["time"];
            std::string contactee = fields["contactee"];

            if(!date.empty()){
                db->deleteAppointment(date, time, contactee);
                db->saveToFile();
            }

            response = "HTTP/1.1 303 See Other\r\nLocation: /\r\nContent-Length: 0\r\n\r\n";
        } else if(request.find("/addAppointment") != std::string::npos){

            
            size_t h_end = request.find("\r\n\r\n");
            std::string headers = (h_end==std::string::npos) ? std::string() : request.substr(0, h_end);
            std::string body = (h_end==std::string::npos) ? std::string() : request.substr(h_end+4);
            std::map<std::string,std::string> fields;
            if(headers.find("multipart/form-data") != std::string::npos){
                size_t bpos = headers.find("boundary=");
                if(bpos != std::string::npos){
                    std::string boundary = headers.substr(bpos + 9);
                    auto semi = boundary.find_first_of("\r\n; ");
                    if(semi != std::string::npos) boundary = boundary.substr(0, semi);
                    fields = parse_multipart(body, boundary);
                } else {
                    fields = parse_urlencoded(body);
                }
            } else {
                fields = parse_urlencoded(body);
            }
            // Debug: print parsed field names and sizes
            
            std::string date = fields["date"];
            std::string time = fields["time"];
            std::string contactee = fields["contactee"];
            std::string location = fields["location"];
            std::string description = fields["description"];
            std::string imagePath;
            // Prefer uploaded file field; fall back to imagePath field
            std::string rawImage;
            if (fields.find("image") != fields.end() && !fields["image"].empty()) rawImage = fields["image"];
            else rawImage = fields["imagePath"];

            // Normalize: if rawImage is a data URL or looks like base64, decode and save into uploads/
            if (!rawImage.empty()) {
                // data URL: data:[<mediatype>][;base64],<data>
                if (rawImage.rfind("data:", 0) == 0) {
                    size_t comma = rawImage.find(',');
                    std::string meta = (comma==std::string::npos) ? std::string() : rawImage.substr(5, comma-5);
                    std::string b64 = (comma==std::string::npos) ? std::string() : rawImage.substr(comma+1);
                    std::string bytes = base64_decode(b64);
                    mkdir("uploads", 0755);
                    std::string ext = ".jpg";
                    if (meta.find("image/png") != std::string::npos) ext = ".png";
                    std::string uniq = std::to_string(std::time(nullptr)) + "_" + std::to_string(rand() % 100000);
                    std::string savedPath = std::string("uploads/img_") + uniq + ext;
                    std::ofstream ofs(savedPath, std::ios::binary);
                    if (ofs) { ofs.write(bytes.data(), bytes.size()); ofs.close(); imagePath = std::string("/") + savedPath; }
                }
                // looks like raw base64 (not data:), decode and save
                else if (rawImage.size() > 100 || rawImage.find('=') != std::string::npos || rawImage.find('+') != std::string::npos) {
                    std::string bytes = base64_decode(rawImage);
                    mkdir("uploads", 0755);
                    std::string ext = ".jpg";
                    std::string uniq = std::to_string(std::time(nullptr)) + "_" + std::to_string(rand() % 100000);
                    std::string savedPath = std::string("uploads/img_") + uniq + ext;
                    std::ofstream ofs(savedPath, std::ios::binary);
                    if (ofs) { ofs.write(bytes.data(), bytes.size()); ofs.close(); imagePath = std::string("/") + savedPath; }
                }
                else {
                    // assume it's a path; ensure leading slash
                    imagePath = rawImage;
                    if (imagePath.rfind("/", 0) != 0) imagePath = std::string("/") + imagePath;
                }
            }

            if(!date.empty() && !time.empty() && !contactee.empty()){
                Appointment apt(date,time,contactee,location,description,imagePath);
                db->addAppointment(apt);
                db->saveToFile();
                
            }
            response = "HTTP/1.1 303 See Other\r\nLocation: /\r\nContent-Length: 0\r\n\r\n";
        
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





