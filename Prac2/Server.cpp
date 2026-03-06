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

#include "database.h"

#define ESC "\x1b"

class server
{
private:
    std::atomic<bool> running{false};
    int server_no = -1;
    std::set<int> client;
    std::mutex clients_mutex;
    Database *db;
    struct sockaddr_in address; // Moved address to class member so start function can use for connections

    void clearScreen(int socket)
    {
        std::string cmd = std::string(ESC) + "[2J" + ESC + "[H";
        send(socket, cmd.c_str(), cmd.length(), 0);
    }

    void moveCursor(int socket, int row, int col)
    {
        std::string cmd = std::string(ESC) + "[" + std::to_string(row) +
                          ";" + std::to_string(col) + "H";
        send(socket, cmd.c_str(), cmd.length(), 0);
    }

    void setcolour(int socket, int colour)
    { // Colour
        std::string cmd = std::string(ESC) + "[" + std::to_string(colour) + "m";
        send(socket, cmd.c_str(), cmd.length(), 0);
    }

    void help(int socket)
    {
        setcolour(socket, 33);
        sendString(socket, "Help:");
        sendString(socket, "1. Add Appointment");
        sendString(socket, "2. View All Appointments");
        sendString(socket, "3. Search Appointments");
        sendString(socket, "4. Delete Appointment");
        sendString(socket, "5. Exit");
        sendString(socket, "");
        resetcolour(socket);
    }

    void resetcolour(int socket)
    {
        std::string cmd = std::string(ESC) + "[0m";
        send(socket, cmd.c_str(), cmd.length(), 0);
    }

    void sendString(int socket, const std::string &str)
    {
        // Add \r\n for proper telnet line endings
        std::string output = str + "\r\n";
        send(socket, output.c_str(), output.length(), 0);
    }

    // std::string readLine(int socket)
    // {
    //     std::string line;
    //     char c;
    //     while (true)
    //     {
    //         int n = recv(socket, &c, 1, 0);
    //         if (n <= 0)
    //             break;

    //         unsigned char uc = static_cast<unsigned char>(c);
    //         if (uc == 255)
    //         { // IAC - telnet negotiation
    //             char discard[2];
    //             recv(socket, discard, 2, 0);
    //             continue;
    //         }

    //         if (c == '\r' || c == '\n')
    //         {
    //             // if (line.empty())
    //             // {
    //             //     continue;
    //             // }
    //             send(socket, "\r\n", 2, 0); // Proper line ending
    //             break;
    //         }
    //         else if (c == 127 || c == 8)
    //         { // Backspace/Delete
    //             if (!line.empty())
    //             {
    //                 line.pop_back();
    //                 send(socket, "\b \b", 3, 0); // Erase character
    //             }
    //         }
    //         else if (static_cast<unsigned char>(c) == 27)
    //         {                     // ESC
    //             return "__ESC__"; // special token
    //         }
    //         else
    //         {
    //             line += c;
    //             //send(socket, &c, 1, 0); // Echo character back
    //         }
    //     }
    //     // std::cout << "Received line: " << line << std::endl; // Debug output
    //     return line;
    // }

    std::string readLine(int socket)
    {
        std::string line;
        char c;

        while (true)
        {
            int n = recv(socket, &c, 1, 0);
            if (n <= 0)
                break;

            unsigned char uc = static_cast<unsigned char>(c);

            // Handle Telnet negotiation (IAC)
            if (uc == 255)
            {
                char discard[2];
                recv(socket, discard, 2, 0);
                continue;
            }

            // Handle carriage return (CR)
            if (c == '\r')
            {
                // Swallow following '\n' if present (Telnet sends CRLF)
                char next;
                int m = recv(socket, &next, 1, MSG_PEEK);
                if (m > 0 && next == '\n')
                    recv(socket, &next, 1, 0);

                send(socket, "\r\n", 2, 0);
                break; // Allow empty input
            }

            // Handle lone newline
            if (c == '\n')
            {
                send(socket, "\r\n", 2, 0);
                break;
            }

            // Handle backspace/delete
            if (c == 127 || c == 8)
            {
                if (!line.empty())
                {
                    line.pop_back();
                    send(socket, "\b \b", 3, 0); // erase character visually
                }
                continue;
            }

            // Handle ESC key
            if (uc == 27)
            {
                return "__ESC__";
            }

            // Normal character
            line += c;
            send(socket, &c, 1, 0); // Echo back (remove if client does local echo)
        }

        return line;
    }

    void viewAllAppointments(int client_socket)
    {
        clearScreen(client_socket);
        sendString(client_socket, "=== All Appointments ===");
        sendString(client_socket, "");

        std::string output = db->printAll();
        if (output.empty())
        {
            sendString(client_socket, "No appointments found.");
        }
        else
        {
            sendString(client_socket, output);
        }

        sendString(client_socket, "Press ENTER to continue...");
        readLine(client_socket);
    }

    bool validateDate(const std::string &date)
    {
        static const std::regex pattern("^(0[1-9]|[12][0-9]|3[01])/(0[1-9]|1[0-2])/[0-9]{4}$");
        return std::regex_match(date, pattern);
    }

    bool validateTime(const std::string &time)
    {
        static const std::regex pattern("^([01][0-9]|2[0-3]):([0-5][0-9])$");
        return std::regex_match(time, pattern);
    }

    void handleClient(int socket)
    {
        {
            std::lock_guard<std::mutex> lock(clients_mutex);
            client.insert(socket);
        }

        clearScreen(socket);
        setcolour(socket, 36);
        sendString(socket, "Welcome to the Appointment Scheduler Server!");
        resetcolour(socket);
        sendString(socket, "Type 'help' for a list of commands.");
        sendString(socket, "Type 'stop' in the server terminal to shut down the server.");
        sendString(socket, "");
        while (running)
        {
            send(socket, "Enter command: ", 15, 0);
            std::string command = readLine(socket);
            if (command.empty())
            {
                continue;
            }

            if (command == "__ESC__")
            {
                continue;
            }

            if (command == "help")
            {
                help(socket);
            }
            else if (command == "1." || command == "1" || command == "add")
            {
                sendString(socket, "Adding an appointment...");
                // Problems : Double print to terminal and Enter does not count as empty input.
                std::string date, time, contactee, location, description;

                sendString(socket, "Please enter the Date (dd/mm/yyyy) of the Appointment.");
                date = readLine(socket);

                if (date == "__ESC__")
                {
                    continue;
                }

                sendString(socket, "Please enter the Time (hh:mm) of the Appointment.");
                time = readLine(socket);

                if (time == "__ESC__")
                {
                    continue;
                }

                sendString(socket, "Please enter the Contactee of the Appointment (Required).");
                contactee = readLine(socket);

                if (contactee == "__ESC__")
                {
                    continue;
                }

                sendString(socket, "Please enter the Location of the Appointment (Optional).");
                location = readLine(socket);

                if (location == "__ESC__")
                {
                    continue;
                }

                sendString(socket, "Please enter the Description of the Appointment (Optional).");
                description = readLine(socket);

                if (description == "__ESC__")
                {
                    continue;
                }

                if (!validateDate(date))
                {
                    sendString(socket, "Invalid date format. Please use dd/mm/yyyy.");
                    continue;
                }

                if (!validateTime(time))
                {
                    sendString(socket, "Invalid time format. Please use hh:mm in 24-hour format.");
                    continue;
                }

                if (contactee.empty())
                {
                    sendString(socket, "Contactee cannot be empty.");
                    continue;
                }

                Appointment a(date, time, contactee, location, description);
                db->addAppointment(a);

                std::cout << "Adding appointment command received\n"; // Debug output
            }
            else if (command == "2." || command == "2" || command == "view")
            {
                sendString(socket, "Viewing all appointments...");
                viewAllAppointments(socket);
            }
            else if (command == "3." || command == "3" || command == "search")
            {
                sendString(socket, "Searching appointments...");

                std::string search;

                sendString(socket, "Please enter your search criteria.");
                search = readLine(socket);

                if (search == "__ESC__")
                {
                    continue;
                }

                std::vector<Appointment *> results = db->search(search);

                if (results.empty())
                {
                    sendString(socket, "0 results found.");
                    continue;
                }
                else
                {
                    sendString(socket, std::to_string(results.size()) + " results found...");

                    for (auto *it : results)
                    {
                        sendString(socket, it->toString());
                    }
                }
                // not working
            }
            else if (command == "4." || command == "4" || command == "delete")
            {
                sendString(socket, "Deleting an appointment...");

                sendString(socket, "Adding an appointment...");

                std::string date, time, contactee;

                sendString(socket, "Please enter the Date (dd/mm/yyyy) of the Appointment.");
                date = readLine(socket);

                if (date == "__ESC__")
                {
                    continue;
                }

                sendString(socket, "Please enter the Time (hh:mm) of the Appointment.");
                time = readLine(socket);

                if (time == "__ESC__")
                {
                    continue;
                }

                sendString(socket, "Please enter the Contactee of the Appointment (Required).");
                contactee = readLine(socket);

                if (contactee == "__ESC__")
                {
                    continue;
                }

                if (!validateDate(date))
                {
                    sendString(socket, "Invalid date format. Please use dd/mm/yyyy.");
                    continue;
                }

                if (!validateTime(time))
                {
                    sendString(socket, "Invalid time format. Please use hh:mm in 24-hour format.");
                    continue;
                }

                if (contactee.empty())
                {
                    sendString(socket, "Contactee cannot be empty.");
                    continue;
                }

                db->deleteAppointment(date, time, contactee);
            }
            else if (command == "5." || command == "5" || command == "exit")
            {
                sendString(socket, "Exiting the server.");
                break;
            }
            else
            {
                sendString(socket, "Invalid command. Type 'help' for a list of commands.");
            }
        }

        {
            std::lock_guard<std::mutex> lock(clients_mutex);
            client.erase(socket);
        }

        shutdown(socket, SHUT_RDWR);
        close(socket);
    }

public:
    server(int port, const std::string &filename)
    {
        db = new Database(filename);
        std::memset(&address, 0, sizeof(address)); // Zeroing out memory to clear garabge

        if ((server_no = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            std::cerr << "Socket creation failed: " << std::strerror(errno) << "\n";
            exit(EXIT_FAILURE);
        }

        int opt = 1;
        setsockopt(server_no, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);

        // Bind
        if (bind(server_no, (struct sockaddr *)&address, sizeof(address)) < 0)
        {
            std::cerr << "Bind failed: " << std::strerror(errno) << "\n";
            exit(EXIT_FAILURE);
        }

        if (listen(server_no, 3) < 0)
        {
            std::cerr << "Listen failed: " << std::strerror(errno) << "\n";
            exit(EXIT_FAILURE);
        }

        std::cout << "Server listening on port " << port << std::endl;
    }

    ~server()
    {
        if (server_no >= 0)
        {
            close(server_no);
        }
        delete db;
    }

    void stop()
    {
        running = false;

        {
            std::lock_guard<std::mutex> lock(clients_mutex);
            for (std::set<int>::iterator it = client.begin(); it != client.end(); ++it)
            {
                shutdown(*it, SHUT_RDWR);
                close(*it);
            }
            client.clear();
        }

        if (server_no >= 0)
        {
            shutdown(server_no, SHUT_RDWR);
            close(server_no);
            server_no = -1;
        }
    }

    void start()
    {
        int addrlen = sizeof(address);
        running = true;

        while (running)
        {
            int client_socket = accept(server_no, (struct sockaddr *)&address,
                                       (socklen_t *)&addrlen);
            if (client_socket < 0)
            {
                if (!running || errno == EINTR || errno == EBADF)
                {
                    break;
                }
                std::cerr << "Accept failed\n";
                continue;
            }

            // Send telnet negotiation for dealing with the echoing problem
            unsigned char dont_echo[] = {255, 254, 1}; // IAC DONT ECHO

            unsigned char do_sga[] = {255, 253, 3}; // IAC DO SUPPRESS-GO-AHEAD
            // The suppress go ahead is getting ridge of the garadge characters from the Don't echo

            send(client_socket, dont_echo, sizeof(dont_echo), 0);

            send(client_socket, do_sga, sizeof(do_sga), 0);

            std::thread client_thread([this, client_socket]()
                                      {
                    std::cout << "Client connected\n";
                    handleClient(client_socket);
                    std::cout << "Client disconnected\n"; });
            client_thread.detach();
        }
        std::cout << "Server shutting down...\n";
    }
};

int main(int argc, char **argv)
{
    uint16_t port = 8080;
    if (argc > 1)
    {
        port = static_cast<uint16_t>(std::stoi(argv[1]));
    } // This is for choosing the port that the server should  listen when the server is started.

    server s(port, "bookings.txt");

    std::thread shutdown_listener([&s]()
                                  {
        std::string command;
        while (std::getline(std::cin, command)) {
            if (command == "stop") {
                std::cout << "Stopping server from terminal command...\n";
                s.stop();
                break;
            }
        } });
    std::cout << "Type 'stop' in this terminal to shut down the server.\n";
    shutdown_listener.detach();
    s.start();

    return 0;
}
