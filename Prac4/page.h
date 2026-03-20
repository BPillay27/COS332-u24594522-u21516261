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
    void updateAppointments();

private:
    
    std::string html;
};
