#include "page.h"

// populateTable
// update generic to a "home page".

Page::Page()
{
    html = "<!DOCTYPE html>"
           "<html>"
           "<head>"
           "<meta charset=\"UTF-8\">"
           "<title>Appointments</title>"
           "</head>"
           "<body>";

    html += "</body></html>";
}

Page::~Page()
{
    if(!days.empty()){
        for(day* _days : days){
            if(_days !=nullptr){
                delete _days;
            }
        }

        days.clear();
    }
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

    std::string rows = "";

    for (size_t i = 0; i < days.size(); i++)
    {
        std::vector<Appointment *> appointments = days[i]->getAppointments();

        for (size_t j = 0; j < appointments.size(); j++)
        {
            Appointment *apt = appointments[j];

            if (apt == nullptr)
            {
                continue;
            }

            rows += "          <tr>\n";
            rows += "            <td>" + apt->getDate() + "</td>\n";
            rows += "            <td>" + apt->getTime() + "</td>\n";
            rows += "            <td>" + apt->getContactee() + "</td>\n";
            rows += "            <td>" + apt->getLocation() + "</td>\n";
            rows += "            <td>" + apt->getDescription() + "</td>\n";
            rows += "            <td>" + apt->getImagePath() + "</td>\n";
            rows += "            <td>\n";
            rows += "              <form action=\"/deleteAppointment\" method=\"POST\" style=\"margin:0;\">\n";
            rows += "                <input type=\"hidden\" name=\"date\" value=\"" + apt->getDate() + "\" />\n";
            rows += "                <input type=\"hidden\" name=\"time\" value=\"" + apt->getTime() + "\" />\n";
            rows += "                <input type=\"hidden\" name=\"contactee\" value=\"" + apt->getContactee() + "\" />\n";
            rows += "                <button type=\"submit\" style=\"background:#ff3b3b; border:none; border-radius:12px; padding:4px 12px;\">Delete</button>\n";
            rows += "              </form>\n";
            rows += "            </td>\n";
            rows += "          </tr>\n";
        }
    }

    if (rows.empty())
    {
        rows += "          <tr>\n";
        rows += "            <td colspan=\"7\">No appointments found.</td>\n";
        rows += "          </tr>\n";
    }

    page += "<!DOCTYPE html>\n";
    page += "<html lang=\"en\">\n";
    page += "<head>\n";
    page += "  <meta charset=\"UTF-8\" />\n";
    page += "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\" />\n";
    page += "  <title>Appointments</title>\n";
    page += "</head>\n";

    page += "<body style=\"margin:0; background:#efefef; font-family:Arial, sans-serif;\">\n";

    page += "  <div style=\"width:1300px; margin:30px auto;\">\n";

    page += "    <h1 style=\"text-align:center; font-size:50px; font-weight:normal; margin:0 0 20px 0;\">Appointments</h1>\n";

    // Add form
    page += "    <div style=\"background:#cfcfcf; padding:40px 35px; min-height:250px; box-sizing:border-box; position:relative;\">\n";
    page += "      <form action=\"/addAppointment\" method=\"POST\" enctype=\"multipart/form-data\">\n";

    page += "        <div style=\"width:650px;\">\n";

    page += "          <div style=\"margin-bottom:16px;\">\n";
    page += "            <label style=\"display:inline-block; width:270px; font-size:24px;\">Date:</label>\n";
    page += "            <input name=\"date\" type=\"text\" style=\"width:350px; height:32px; border:none; border-radius:18px; background:#f4f4f4;\" />\n";
    page += "          </div>\n";

    page += "          <div style=\"margin-bottom:16px;\">\n";
    page += "            <label style=\"display:inline-block; width:270px; font-size:24px;\">Time:</label>\n";
    page += "            <input name=\"time\" type=\"text\" style=\"width:350px; height:32px; border:none; border-radius:18px; background:#f4f4f4;\" />\n";
    page += "          </div>\n";

    page += "          <div style=\"margin-bottom:16px;\">\n";
    page += "            <label style=\"display:inline-block; width:270px; font-size:24px;\">Contactee:</label>\n";
    page += "            <input name=\"contactee\" type=\"text\" style=\"width:350px; height:32px; border:none; border-radius:18px; background:#f4f4f4;\" />\n";
    page += "          </div>\n";

    page += "          <div style=\"margin-bottom:16px;\">\n";
    page += "            <label style=\"display:inline-block; width:270px; font-size:24px;\">Location</label>\n";
    page += "            <input name=\"location\" type=\"text\" style=\"width:350px; height:32px; border:none; border-radius:18px; background:#f4f4f4;\" />\n";
    page += "          </div>\n";

    page += "          <div style=\"margin-bottom:16px;\">\n";
    page += "            <label style=\"display:inline-block; width:270px; font-size:24px;\">Description</label>\n";
    page += "            <input name=\"description\" type=\"text\" style=\"width:350px; height:32px; border:none; border-radius:18px; background:#f4f4f4;\" />\n";
    page += "          </div>\n";

    page += "          <div style=\"margin-bottom:16px;\">\n";
    page += "            <label style=\"display:inline-block; width:270px; font-size:24px;\">Image</label>\n";
    page += "            <input name=\"image\" type=\"file\" accept=\"image/*\" style=\"width:350px; border-radius:18px; background:#f4f4f4;\" />\n";
    page += "          </div>\n";

    page += "        </div>\n";

    page += "        <div style=\"position:absolute; top:18px; right:40px;\">\n";
    page += "          <button type=\"submit\" style=\"background:#ff3b3b; border:none; border-radius:18px; padding:8px 24px; font-size:20px; margin-left:14px;\">Add</button>\n";
    page += "        </div>\n";

    page += "      </form>\n";
    page += "    </div>\n";

    // Search form
    page += "    <div style=\"background:#cfcfcf; margin-top:20px; padding:20px 25px; box-sizing:border-box;\">\n";
    page += "      <form action=\"/searchAppointments\" method=\"GET\">\n";
    page += "        <label style=\"display:inline-block; width:160px; font-size:24px;\">Search:</label>\n";
    page += "        <input name=\"keyword\" type=\"text\" placeholder=\"Search appointments...\" ";
    page += "style=\"width:350px; height:32px; border:none; border-radius:18px; background:#f4f4f4; padding-left:10px;\" />\n";
    page += "        <button type=\"submit\" style=\"background:#ff3b3b; border:none; border-radius:18px; padding:8px 24px; font-size:20px; margin-left:14px;\">Search</button>\n";
    page += "      </form>\n";
    page += "    </div>\n";

    // Table
    page += "    <div style=\"background:#cfcfcf; margin-top:45px; padding:20px 25px 40px 25px; min-height:500px;\">\n";

    page += "      <table style=\"width:100%; border-collapse:collapse; font-size:22px;\">\n";
    page += "        <thead>\n";
    page += "          <tr>\n";
    page += "            <th style=\"text-align:left; padding-bottom:20px;\">Date</th>\n";
    page += "            <th style=\"text-align:left; padding-bottom:20px;\">Time</th>\n";
    page += "            <th style=\"text-align:left; padding-bottom:20px;\">Contactee</th>\n";
    page += "            <th style=\"text-align:left; padding-bottom:20px;\">Location</th>\n";
    page += "            <th style=\"text-align:left; padding-bottom:20px;\">Description</th>\n";
    page += "            <th style=\"text-align:left; padding-bottom:20px;\">Image</th>\n";
    page += "            <th style=\"text-align:left; padding-bottom:20px;\">Delete</th>\n";
    page += "          </tr>\n";
    page += "        </thead>\n";

    page += "        <tbody>\n";
    page += rows;
    page += "        </tbody>\n";

    page += "      </table>\n";

    page += "    </div>\n";

    page += "  </div>\n";

    page += "</body>\n";
    page += "</html>\n";
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
           "<title>Appointments</title>"
           "</head>"
           "<body>";

    html += "</body></html>";
}

std::string Page::getHTML()
{
    return html;
}

bool Page::compareDates(const std::string &a, const std::string &b)
{
    int d1, m1, y1;
    int d2, m2, y2;

    sscanf(a.c_str(), "%d/%d/%d", &d1, &m1, &y1);
    sscanf(b.c_str(), "%d/%d/%d", &d2, &m2, &y2);

    if (y1 != y2)
        return y1 < y2;
    if (m1 != m2)
        return m1 < m2;
    return d1 < d2;
}



bool Page::compareTimes(const std::string &a, const std::string &b)
{
    int h1, m1;
    int h2, m2;

    sscanf(a.c_str(), "%d:%d", &h1, &m1);
    sscanf(b.c_str(), "%d:%d", &h2, &m2);

    if (h1 != h2)
        return h1 < h2;

    return m1 < m2;
}

bool Page::compareDateTime(const Appointment *a, const Appointment *b)
{
    // compare dates first
    if (a->getDate() != b->getDate())
    {
        return compareDates(a->getDate(), b->getDate());
    }

    // if same date → compare time
    return compareTimes(a->getTime(), b->getTime());
}

void Page::updateDays(std::vector<Appointment *> _appointments)
{
    for (day *d : days)
    {
        delete d;
    }
    days.clear();

    if (_appointments.empty())
    {
        return;
    }

    std::vector<Appointment *> appointments;

    for (Appointment *app : _appointments)
    {
        if (app != nullptr)
        {
            appointments.push_back(new Appointment(app->getDate(), app->getTime(), app->getContactee(), app->getLocation(), app->getDescription(), app->getImageStored()));
        }
    }

    std::sort(appointments.begin(), appointments.end(), [this](Appointment *a, Appointment *b) {
        return compareDateTime(a, b);
    });

    std::string currentDate = "";
    day *currentDay = nullptr;

    for (Appointment *app : appointments)
    {
        if (app == nullptr)
            continue;

        if (currentDate != app->getDate())
        {
            currentDate = app->getDate();
            currentDay = new day(currentDate);
            days.push_back(currentDay);
        }

        currentDay->addAppointment(*app);
    }
}

void Page::updateDays(std::vector<day *> newDays)
{
    /*
    for (day *oldDay : days)
    {
        delete oldDay;
    }
    Don't delete my days you mad man    */
    days.clear();

    for (day *sourceDay : newDays)
    {
        if (sourceDay == nullptr)
        {
            continue;
        }

        day *copiedDay = new day(sourceDay->getDate());

        std::vector<Appointment *> sourceAppointments = sourceDay->getAppointments();
        std::vector<Appointment *> copiedAppointments;

        for (Appointment *app : sourceAppointments)
        {
            if (app == nullptr)
            {
                continue;
            }

            Appointment *copiedApp = new Appointment(app->getDate(), app->getTime(), app->getContactee(), app->getLocation(), app->getDescription(), app->getImageStored());
            copiedAppointments.push_back(copiedApp);
        }

        std::sort(copiedAppointments.begin(), copiedAppointments.end(),
                  [this](Appointment *a, Appointment *b)
                  {
                      if (a == nullptr || b == nullptr)
                      {
                          return false;
                      }

                      return compareTimes(a->getTime(), b->getTime());
                  });

        for (Appointment *app : copiedAppointments)
        {
            if (app == nullptr)
            {
                continue;
            }

            copiedDay->addAppointment(*app);
            delete app;
        }

        days.push_back(copiedDay);
    }

    std::sort(days.begin(), days.end(), [this](day *a, day *b)
              {
                  if (a == nullptr || b == nullptr)
                  {
                      return false;
                  }

                  return compareDates(a->getDate(), b->getDate()); });
}
