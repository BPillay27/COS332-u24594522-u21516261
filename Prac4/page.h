#include <iostream>
#include <string>
#include <map>
#include <ctime>

#include "database.h"

class Page
{
public:
    Page();
    ~Page();
    std::string generateGeneric(); // change to make Generic
    void appendHTML(const std::string &content);
    void clearPage();
    std::time_t getCurrentTime() const;
    std::string convertTimeToString(std::time_t time) const;
    std::string getHTML();
    bool updateAppointments(); // Needs to be called 1st.
    bool addAppointment(const Appointment &apt);
    bool deleteAppointment(const Appointment &apt);
    bool searchAppointments(const std::string &keyword);

private:
    std::vector<day> days;
    std::string html;
};
