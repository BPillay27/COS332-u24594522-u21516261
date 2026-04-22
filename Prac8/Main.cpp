#include "Server.h"
#include <string>
#include <thread>
#include <chrono>

int main()
{
    Server server("watch", "backup");

    server.backupFiles();

    while (true)
    {
        if (server.checkForChanges())
        {
            server.backupFiles();
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    
    return 0;
}