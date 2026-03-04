#include "page.h"

Page::Page()
{
    cityNames[Johannesburg] = "Johannesburg";
    cityNames[NewYork] = "New York";
    cityNames[London] = "London";
    cityNames[Tokyo] = "Tokyo";
    cityNames[Frankfurt] = "Frankfurt";
    cityNames[Sydney] = "Sydney";
}

Page::~Page()
{
    // Destructor logic if needed
}

std::time_t Page::getCurrentTime() const
{
    return std::time(NULL);
}

std::time_t Page::getCityTime(Cities city) const
{
    std::time_t currentTime = getCurrentTime();
    std::tm timeInfo = *std::localtime(&currentTime);

    switch (city)
    {
    case Johannesburg:
        timeInfo.tm_hour += 0; // This is the base time (UTC+2), so no adjustment needed
        break;
    case NewYork:
        timeInfo.tm_hour -= 7;
        break;
    case London:
        timeInfo.tm_hour -= 2;
        break;
    case Tokyo:
        timeInfo.tm_hour += 7;
        break;
    case Frankfurt:
        timeInfo.tm_hour -= 1;
        break;
    case Sydney:
        timeInfo.tm_hour += 8;
        break;
    default:
        return currentTime;
    }

    return std::mktime(&timeInfo);
}

std::string Page::convertTimeToString(std::time_t time) const
{
    std::tm *timeInfo = std::localtime(&time);
    char buffer[80];
    std::strftime(buffer, sizeof(buffer), "%H:%M:%S", timeInfo);
    return std::string(buffer);
}

void Page::generatePage()
{
}
