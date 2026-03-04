#include "page.h"

Page::Page()
{
    // Initialize citySelected array to false
    for (int i = 0; i < CITY_COUNT; ++i)
    {
        citySelected[i] = false;
    }

    citySelected[Johannesburg] = true; // Johannesburg is selected by default
}

Page::~Page()
{
    // Destructor logic if needed
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

std::string Page::generatePage()
{
    std::string html;

    html += "<html><head>";
    html += "<meta http-equiv=\"refresh\" content=\"1\">";
    html += "<title>World Clock</title>";

    html += "<style>";
    html += "body { font-family: Arial, sans-serif; text-align: center; background:#f5f5f5; }";
    html += "h1 { margin-top: 30px; }";
    html += "table { margin: auto; border-collapse: collapse; }";
    html += "td { padding: 15px 40px; font-size: 18px; }";
    html += "a { text-decoration: none; color: #0066cc; font-weight: bold; }";
    html += "a:hover { text-decoration: underline; }";
    html += "button { padding:10px 20px; font-size:16px; margin-top:20px; cursor:pointer; }";
    html += "</style>";

    html += "</head><body>";

    html += "<h1>World Clock</h1>";
    html += "<table>";

    for (int i = 0; i < CITY_COUNT; i++)
    {
        html += "<tr>";

        html += "<td>";

        html += "<a href=\"/?city=";
        html += char('0' + i);
        html += "\">";

        switch (i)
        {
        case Johannesburg:
            html += "Johannesburg";
            break;
        case NewYork:
            html += "New York";
            break;
        case London:
            html += "London";
            break;
        case Tokyo:
            html += "Tokyo";
            break;
        case Frankfurt:
            html += "Frankfurt";
            break;
        case Sydney:
            html += "Sydney";
            break;
        }

        html += "</a>";

        if (citySelected[i])
        {
            html += " : ";
            html += convertTimeToString(getCityTime((Cities)i));
        }

        html += "</td>";

        if (citySelected[i])
        {
            int next = -1;

            for (int j = i + 1; j < CITY_COUNT; j++)
            {
                if (citySelected[j])
                {
                    next = j;
                    break;
                }
            }

            if (next != -1)
            {
                html += "<td>";

                html += "<a href=\"/?city=";
                html += char('0' + next);
                html += "\">";

                switch (next)
                {
                case Johannesburg:
                    html += "Johannesburg";
                    break;
                case NewYork:
                    html += "New York";
                    break;
                case London:
                    html += "London";
                    break;
                case Tokyo:
                    html += "Tokyo";
                    break;
                case Frankfurt:
                    html += "Frankfurt";
                    break;
                case Sydney:
                    html += "Sydney";
                    break;
                }

                html += "</a>";

                html += " : ";
                html += convertTimeToString(getCityTime((Cities)next));

                html += "</td>";

                i = next;
            }
        }

        html += "</tr>";
    }

    html += "</table>";
    html += "<br><button>Reset</button>";
    html += "</body></html>";

    return html;
}
