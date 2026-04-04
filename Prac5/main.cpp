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

    // Use admin bind so add/delete work
    if (!client.simpleBind("cn=admin,dc=assets,dc=local", "2704"))
    {
        std::cerr << "Bind failed.\n";
        return 1;
    }

    while (true)
    {
        std::cout << "\n==== LDAP Client ====\n";
        std::cout << "1. Search plane\n";
        std::cout << "2. Add plane\n";
        std::cout << "3. Delete plane\n";
        std::cout << "4. Exit\n";
        std::cout << "Choice: ";

        std::string choice;
        std::getline(std::cin, choice);

        if (choice == "1")
        {
            std::string name;
            std::cout << "Enter plane name: ";
            std::getline(std::cin, name);

            std::string speed = client.searchPlaneSpeed(name);

            if (speed.empty())
                std::cout << "Plane not found.\n";
            else
                std::cout << "Maximum speed: " << speed << " km/h\n";
        }
        else if (choice == "2")
        {
            std::string name, speed;

            std::cout << "Enter plane name: ";
            std::getline(std::cin, name);

            std::cout << "Enter max speed: ";
            std::getline(std::cin, speed);

            if (client.addPlane(name, speed))
                std::cout << "Plane added.\n";
            else
                std::cout << "Add failed.\n";
        }
        else if (choice == "3")
        {
            std::string name;
            std::cout << "Enter plane name to delete: ";
            std::getline(std::cin, name);

            if (client.deletePlane(name))
                std::cout << "Plane deleted.\n";
            else
                std::cout << "Delete failed.\n";
        }
        else if (choice == "4")
        {
            break;
        }
        else
        {
            std::cout << "Invalid option.\n";
        }
    }

    client.closeConnection();
    return 0;
}