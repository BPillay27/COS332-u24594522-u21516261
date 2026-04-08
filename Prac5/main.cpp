#include "LDAPClient.h"
#include <iostream>
#include <vector>

int main()
{
    LDAPClient client("127.0.0.1", 389);

    if (!client.connectToServer())
    {
        std::cerr << "Failed to connect to server." << std::endl;
        return 1;
    }

    if (!client.simpleBind("cn=admin,dc=assets,dc=local", "2704"))
    {
        std::cerr << "Anonymous bind failed." << std::endl;
        client.closeConnection();
        return 1;
    }

    int choice = 0;

    do
    {
        std::cout << "\n=== LDAP Plane Client ===" << std::endl;
        std::cout << "1. Search plane" << std::endl;
        std::cout << "2. Add plane" << std::endl;
        std::cout << "3. Delete plane" << std::endl;
        std::cout << "4. Get all planes" << std::endl;
        std::cout << "5. Exit" << std::endl;
        std::cout << "Enter choice: ";
        std::cin >> choice;

        if (choice == 1)
        {
            std::string planeName;
            std::cout << "Enter plane name: ";
            std::cin >> planeName;

            std::string speed = client.searchPlaneSpeed(planeName);

            if (!speed.empty())
            {
                std::cout << "Plane: " << planeName << ", Speed: " << speed << std::endl;
            }
            else
            {
                std::cout << "Plane not found." << std::endl;
            }
        }
        else if (choice == 2)
        {
            std::string planeName;
            std::string speed;

            std::cout << "Enter plane name: ";
            std::cin >> planeName;

            std::cout << "Enter speed: ";
            std::cin >> speed;

            if (client.addPlane(planeName, speed))
            {
                std::cout << "Plane added successfully." << std::endl;
            }
            else
            {
                std::cout << "Failed to add plane." << std::endl;
            }
        }
        else if (choice == 3)
        {
            std::string planeName;
            std::cout << "Enter plane name: ";
            std::cin >> planeName;

            if (client.deletePlane(planeName))
            {
                std::cout << "Plane deleted successfully." << std::endl;
            }
            else
            {
                std::cout << "Failed to delete plane." << std::endl;
            }
        }
        else if (choice == 4)
        {
            std::vector<PlaneInfo> planes = client.getAllPlanes();

            if (planes.empty())
            {
                std::cout << "No planes found." << std::endl;
            }
            else
            {
                std::cout << "\nAll planes:" << std::endl;
                for (size_t i = 0; i < planes.size(); ++i)
                {
                    std::cout << "Name: " << planes[i].name
                              << ", Speed: " << planes[i].speed << std::endl;
                }
            }
        }
        else if (choice == 5)
        {
            std::cout << "Exiting..." << std::endl;
        }
        else
        {
            std::cout << "Invalid choice." << std::endl;
        }

    } while (choice != 5);

    client.closeConnection();
    return 0;
}