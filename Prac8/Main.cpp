#include "Client.h"
#include <string>
#include <thread>
#include <chrono>
#include <iostream>
#include <atomic>

int main()
{
    BackupClient client("watch", "backup");

    std::atomic<bool> running(false);
    std::atomic<bool> exitProgram(false);

    std::thread worker([&client, &running, &exitProgram]()
    {
        while (!exitProgram)
        {
            if (running)
            {
                if (client.checkForChanges())
                {
                    client.backupFiles();
                }
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });

    std::string command;

    std::cout << "Enter START to begin monitoring or QUIT to exit." << std::endl;

    while (true)
    {
        std::getline(std::cin, command);

        if (command == "START")
        {
            if (!running)
            {
                client.backupFiles(); // initial backup when monitoring starts
                running = true;
                std::cout << "Monitoring started." << std::endl;
            }
            else
            {
                std::cout << "Monitoring is already running." << std::endl;
            }
        }
        else if (command == "QUIT")
        {
            exitProgram = true;
            break;
        }
        else
        {
            std::cout << "Unknown command. Use START or QUIT." << std::endl;
        }
    }

    if (worker.joinable())
    {
        worker.join();
    }

    std::cout << "Program stopped." << std::endl;

    return 0;
}