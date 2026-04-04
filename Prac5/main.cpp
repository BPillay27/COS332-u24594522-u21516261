#include "LDAPClient.h"
#include <iostream>
#include <string>

int main()
{
    LDAPClient client("127.0.0.1", 389);

    if (!client.connectToServer())
    {
        std::cerr << "Could not connect to LDAP server.\n";
        return 1;
    }

    if (!client.anonymousBind())
    {
        std::cerr << "Bind failed.\n";
        return 1;
    }

    while (true)
    {
        std::string planeName;

        std::cout << "\nEnter plane name (or 'exit'): ";
        std::getline(std::cin, planeName);

        if (planeName == "exit")
            break;

        if (planeName.empty())
            continue;

        std::string speed = client.searchPlaneSpeed(planeName);

        if (speed.empty())
        {
            std::cout << "Plane not found or speed missing\n";
        }
        else
        {
            std::cout << "Maximum speed: " << speed << " km/h\n";
        }
    }

    client.closeConnection();
    return 0;
}