#include "page.h"

Page::Page()
{
    // Initialize citySelected array to false
    for (int i = 0; i < CITY_COUNT; ++i)
    {
        citySelected[i] = false;
    }

    citySelected[Johannesburg] = true; // Johannesburg is selected by default

    html = "<!DOCTYPE html>"
           "<html>"
           "<head>"
           "<meta charset=\"UTF-8\">"
           "<meta http-equiv=\"refresh\" content=\"1\">"
           "<title>World Clock</title>"
           "</head>"
           "<body>";

    html += "</body></html>";
}

Page::~Page()
{
    // Destructor logic if needed
}

void Page::resetSelected()
{
    for (int i = 0; i < CITY_COUNT; ++i)
    {
        citySelected[i] = false;
    }

    citySelected[Johannesburg] = true;
}

std::time_t Page::getCityTime(Cities city) const
{
    std::time_t now = std::time(NULL);

    int offsetHours = 0;

    switch (city)
    {
    case Johannesburg:
        offsetHours = 2;
        break;
    case NewYork:
        offsetHours = -5;
        break;
    case London:
        offsetHours = 0;
        break;
    case Tokyo:
        offsetHours = 9;
        break;
    case Frankfurt:
        offsetHours = 1;
        break;
    case Sydney:
        offsetHours = 10;
        break;
    default:
        return now;
    }

    return now + offsetHours * 3600;
}

std::string Page::convertTimeToString(std::time_t time) const
{
    std::tm *timeInfo = std::gmtime(&time);
    char buffer[80];
    std::strftime(buffer, sizeof(buffer), "%H:%M:%S", timeInfo);
    return std::string(buffer);
}

bool Page::selectCity(Cities city)
{
    // Function tries to "turn on" the city. If city was turned on, then return true. Otherwise, return false if the city was already on.
    if (city < 0 || city >= CITY_COUNT)
    {
        return false; // Invalid city
    }

    if (!citySelected[city])
    {
        citySelected[city] = true;
        return true; // City was successfully selected
    }
    else
    {
        return false; // City was already selected
    }
}

bool Page::deselectCity(Cities city)
{
    // Function tries to "turn off" the city. If city was turned off, then return true. Otherwise, return false if the city was already off.
    if (city < 0 || city >= CITY_COUNT || city == Johannesburg)
    {
        return false; // Invalid city
    }

    if (citySelected[city])
    {
        citySelected[city] = false;
        return true; // City was successfully deselected
    }
    else
    {
        return false; // City was already deselected
    }
}

std::string Page::generateGeneric()
{
    std::string page;

    page += "<!DOCTYPE html>";
    page += "<html><head>";
    page += "<meta charset=\"UTF-8\">";
    page += "<meta http-equiv=\"refresh\" content=\"1\">";
    page += "<title>World Clock</title>";

    page += "<style>";
    page += "body { font-family: Arial, sans-serif; text-align: center; background: #f5f5f5; }";
    page += "h1 { margin-top: 30px; }";
    page += "table { margin: auto; border-collapse: collapse; }";
    page += "td { padding: 15px 40px; font-size: 18px; }";
    page += "a.city { text-decoration: none; color: #0066cc; font-weight: bold; }";
    page += "a.city:hover { text-decoration: underline; }";
    page += "a.reset { display: inline-block; padding: 10px 20px; font-size: 16px; ";
    page += "background: #e0e0e0; color: black; text-decoration: none; border: 1px solid #999; ";
    page += "border-radius: 4px; margin-top: 20px; }";
    page += "a.reset:hover { background: #d0d0d0; }";
    page += "</style>";

    page += "</head><body>";

    page += "<h1>World Clock</h1>";
    page += "<table>";

    for (int i = 0; i < CITY_COUNT; i++)
    {
        std::string cityName;
        std::string displayName;

        switch (i)
        {
        case Johannesburg:
            cityName = "Johannesburg";
            displayName = "Johannesburg";
            break;
        case NewYork:
            cityName = "NewYork";
            displayName = "New York";
            break;
        case London:
            cityName = "London";
            displayName = "London";
            break;
        case Tokyo:
            cityName = "Tokyo";
            displayName = "Tokyo";
            break;
        case Frankfurt:
            cityName = "Frankfurt";
            displayName = "Frankfurt";
            break;
        case Sydney:
            cityName = "Sydney";
            displayName = "Sydney";
            break;
        default:
            continue;
        }

        page += "<tr><td>";
        page += "<a class=\"city\" href=\"/";
        if (citySelected[i])
            page += "Deselect/";
        else
            page += "Select/";
        page += cityName;
        page += "\">";
        page += displayName;
        page += "</a>";

        if (citySelected[i])
        {
            page += " : ";
            page += convertTimeToString(getCityTime((Cities)i));
        }

        page += "</td></tr>";
    }

    page += "</table>";
    page += "<br><a class=\"reset\" href=\"/Reset\">Reset</a>";
    page += "</body></html>";

    html = page;
    return page;
}

void Page::appendHTML(const std::string &content)
{
    size_t pos = html.rfind("</body></html>");
    if (pos != std::string::npos)
    {
        html.erase(pos);
    }

    html += content;
    html += "</body></html>";
}

void Page::clearPage()
{
    html = "<!DOCTYPE html>"
           "<html>"
           "<head>"
           "<meta charset=\"UTF-8\">"
           "<meta http-equiv=\"refresh\" content=\"1\">"
           "<title>World Clock</title>"
           "</head>"
           "<body>";

    html += "</body></html>";
}

std::string Page::getHTML()
{
    return html;
}