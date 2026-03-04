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
    void generatePage();
    std::time_t getCurrentTime() const;
    std::time_t getCityTime(Cities city) const;
    std::string convertTimeToString(std::time_t time) const;

private:
    std::map<Cities, std::string> cityNames;
};