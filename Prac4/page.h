#include <iostream>
#include <string>
#include <map>
#include <ctime>

enum Cities
{
    Johannesburg,
    NewYork,
    London,
    Tokyo,
    Frankfurt,
    Sydney,
    CITY_COUNT
};

class Page
{
public:
    Page();
    ~Page();
    std::string generateGeneric(); // change to make Generic
    void appendHTML(const std::string &content);
    void clearPage();
    std::time_t getCurrentTime() const;
    std::time_t getCityTime(Cities city) const;
    std::string convertTimeToString(std::time_t time) const;
    bool selectCity(Cities city);
    bool deselectCity(Cities city);
    void resetSelected();
    std::string getHTML();

private:
    bool citySelected[CITY_COUNT];
    std::string html;
};
