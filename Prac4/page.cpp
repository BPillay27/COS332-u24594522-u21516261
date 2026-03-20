#include "page.h"

// populateTable
// update generic to a "home page".

Page::Page()
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

Page::~Page()
{
    // Destructor logic if needed
}

std::string Page::convertTimeToString(std::time_t time) const
{
    std::tm *timeInfo = std::gmtime(&time);
    char buffer[80];
    std::strftime(buffer, sizeof(buffer), "%H:%M:%S", timeInfo);
    return std::string(buffer);
}

std::string Page::generateGeneric()
{
    std::string page = "";

    page += "<!DOCTYPE html>\n";
    page += "<html lang=\"en\">\n";
    page += "<head>\n";
    page += "  <meta charset=\"UTF-8\" />\n";
    page += "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\" />\n";
    page += "  <title>Appointments Wireframe</title>\n";
    page += "</head>\n";

    page += "<body style=\"margin:0; background:#efefef; font-family:Arial, sans-serif;\">\n";

    page += "  <div style=\"width:1300px; margin:30px auto;\">\n";

    page += "    <h1 style=\"text-align:center; font-size:50px; font-weight:normal; margin:0 0 20px 0;\">Appointments</h1>\n";

    page += "    <div style=\"background:#cfcfcf; padding:40px 35px; min-height:250px; box-sizing:border-box; position:relative;\">\n";

    page += "      <div style=\"width:650px;\">\n";

    page += "        <div style=\"margin-bottom:16px;\">\n";
    page += "          <label style=\"display:inline-block; width:270px; font-size:24px;\">Name:</label>\n";
    page += "          <input type=\"text\" style=\"width:350px; height:32px; border:none; border-radius:18px; background:#f4f4f4;\" />\n";
    page += "        </div>\n";

    page += "        <div style=\"margin-bottom:16px;\">\n";
    page += "          <label style=\"display:inline-block; width:270px; font-size:24px;\">Surname:</label>\n";
    page += "          <input type=\"text\" style=\"width:350px; height:32px; border:none; border-radius:18px; background:#f4f4f4;\" />\n";
    page += "        </div>\n";

    page += "        <div style=\"margin-bottom:16px;\">\n";
    page += "          <label style=\"display:inline-block; width:270px; font-size:24px;\">Date:</label>\n";
    page += "          <input type=\"text\" style=\"width:350px; height:32px; border:none; border-radius:18px; background:#f4f4f4;\" />\n";
    page += "        </div>\n";

     page += "        <div style=\"margin-bottom:16px;\">\n";
    page += "          <label style=\"display:inline-block; width:270px; font-size:24px;\">Time:</label>\n";
    page += "          <input type=\"text\" style=\"width:350px; height:32px; border:none; border-radius:18px; background:#f4f4f4;\" />\n";
    page += "        </div>\n";

    page += "      </div>\n";

    page += "      <div style=\"position:absolute; top:18px; right:40px;\">\n";
    page += "        <button style=\"background:#ff3b3b; border:none; border-radius:18px; padding:8px 24px; font-size:20px; margin-left:14px;\">Add</button>\n";
    page += "        <button style=\"background:#ff3b3b; border:none; border-radius:18px; padding:8px 24px; font-size:20px; margin-left:14px;\">Search</button>\n";
    page += "        <button style=\"background:#ff3b3b; border:none; border-radius:18px; padding:8px 24px; font-size:20px; margin-left:14px;\">Remove</button>\n";
    page += "      </div>\n";

    page += "    </div>\n";

    page += "    <div style=\"background:#cfcfcf; margin-top:45px; padding:20px 25px 40px 25px; min-height:500px;\">\n";

    page += "      <table style=\"width:100%; border-collapse:collapse; font-size:22px;\">\n";
    page += "        <thead>\n";
    page += "          <tr>\n";
    page += "            <th style=\"text-align:left; padding-bottom:20px;\">Date</th>\n";
    page += "            <th style=\"text-align:left; padding-bottom:20px;\">Time</th>\n";
    page += "            <th style=\"text-align:left; padding-bottom:20px;\">Name</th>\n";
    page += "            <th style=\"text-align:left; padding-bottom:20px;\">Surname</th>\n";
    page += "            <th style=\"text-align:left; padding-bottom:20px;\">Doctor Img</th>\n";
    page += "          </tr>\n";
    page += "        </thead>\n";

    page += "        <tbody>\n";
    page += "          <tr>\n";
    page += "            <td>&nbsp;</td>\n";
    page += "            <td></td>\n";
    page += "            <td></td>\n";
    page += "            <td></td>\n";
    page += "            <td></td>\n";
    page += "          </tr>\n";
    page += "        </tbody>\n";

    page += "      </table>\n";

    page += "    </div>\n";

    page += "  </div>\n";

    page += "</body>\n";
    page += "</html>\n";
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
