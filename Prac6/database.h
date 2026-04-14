#include <iostream>
#include <vector>
#include <string>
#include <fstream>

class Appointment
{
private:
    std::string date; // dd/mm/yyyy
    std::string contactee;
public:
    Appointment(std::string date, std::string contactee)
        : date(date), contactee(contactee) {}
    Appointment() : date(""), contactee("") {}
    Appointment(std::string fileString)
    {
        size_t dStart = fileString.find("<D>") + 3;
        size_t dEnd = fileString.find("<D>", dStart);
        date = fileString.substr(dStart, dEnd - dStart);

        size_t cStart = fileString.find("<C>") + 3;
        size_t cEnd = fileString.find("<C>", cStart);
        contactee = fileString.substr(cStart, cEnd - cStart);
    }
    ~Appointment() {}
    std::string getDate() const { return date; }
    std::string getContactee() const { return contactee; }
    std::string toString() const
    {
        ///return "Date: " + date + "  Person: " + contactee + "\n";
        return "--" + contactee + "\n";
    }
    bool search(const std::string &keyword) const
    {
        return date.find(keyword) != std::string::npos ||
               contactee.find(keyword) != std::string::npos;
    } // Helper function for precise searching

    std::string toFileString() const
    {
        return "<D>" + date + "<D><C>" + contactee + "<C>";
    }
};

class day
{
private:
    std::string date; // dd/mm/yyyy
    std::vector<Appointment *> appointments;

public:
    day() : date("") {}
    day(std::string date) : date(date) {}
    void addAppointment(const Appointment &apt)
    {
        appointments.push_back(new Appointment(apt));
    }
    void deleteAppointment(int index)
    {
        if (index >= 0 && index < static_cast<int>(appointments.size()))
        {
            delete appointments[index];
            appointments.erase(appointments.begin() + index);
        }
    }
    ~day()
    {
        for (auto &apt : appointments)
        {
            delete apt;
        }
    }
    std::vector<Appointment *> search(const std::string &keyword, std::vector<Appointment *> &results)
    {
        for (const auto &apt : appointments)
        {
            if (apt->search(keyword))
            {
                results.push_back(apt);
            }
        }
        return results;
    }
    int getIndex(const Appointment &apt)
    {
        for (size_t i = 0; i < appointments.size(); ++i)
        {
            if (appointments[i]->getDate() == apt.getDate() &&
                appointments[i]->getContactee() == apt.getContactee())
            {
                return i;
            }
        }
        return -1; // Not found
    }
    std::string printAppointments() const
    {
        std::string output = "";
        for (const auto &apt : appointments)
        {
            output += apt->toString() + "\n";
        }
        return output;
    }

    std::string getDate() const { return date; }
    const std::vector<Appointment *> &getAppointments() const { return appointments; }
};

class Database
{
private:
    std::vector<day *> days;
    std::string filename;

public:
    Database(std::string filename) : filename(filename)
    {
        // std::cout<<"Making Database\n";
        std::ifstream file(filename);
        std::string line;
        while (std::getline(file, line))
        {
            Appointment apt(line);
            addAppointment(apt);
        }
    }

    std::vector<Appointment *> search(const std::string &keyword)
    {
        std::vector<Appointment *> results;
        if (keyword.empty())
        {
            for (const auto &d : days)
            {
                const auto &appts = d->getAppointments();
                for (const auto &apt : appts)
                {
                    results.push_back(apt);
                }
            }
        }
        else if (keyword.find("/") != std::string::npos)
        {
            for (const auto &d : days)
            {
                if (d->getDate() == keyword)
                {
                    const auto &appts = d->getAppointments();
                    for (const auto &apt : appts)
                    {
                        results.push_back(apt);
                    }
                }
            }
        }
        else
        {
            for (auto &d : days)
            {
                d->search(keyword, results);
            }
        }
        return results;
    }

    void addAppointment(const Appointment &apt)
    {
        for (auto &d : days)
        {
            if (d->getDate() == apt.getDate())
            {
                d->addAppointment(apt);
                return;
            }
        }
        day *newDay = new day(apt.getDate());
        newDay->addAppointment(apt);
        days.push_back(newDay);
    }

    void deleteAppointment(const Appointment &apt)
    {
        for (auto &d : days)
        {
            if (d->getDate() == apt.getDate())
            {
                int index = d->getIndex(apt);
                if (index != -1)
                {
                    d->deleteAppointment(index);
                    return;
                }
            }
        }
    }

    void deleteAppointment(const std::string &date, const std::string &contactee = "")
    {
        day *ResultDate = nullptr;
        for (auto &d : days)
        {
            if (d->getDate() == date)
            {
                ResultDate = d;
                break;
            }
        }

        std::vector<Appointment *> searchResults;
        if (ResultDate != nullptr)
        {
            if (!contactee.empty())
            {
                ResultDate->search(contactee, searchResults);
                
                for (const auto &apt : searchResults){
                    ResultDate->deleteAppointment(ResultDate->getIndex(*apt));
                }
                
            }
            else
            {
                
                for (const auto &apt : searchResults){
                    ResultDate->deleteAppointment(ResultDate->getIndex(*apt));
                }
                
            }
        }
        return;
    }

    std::string printAll() const
    {
        std::string output = "";
        for (const auto &d : days)
        {
            output += "Date: " + d->getDate() + "\n";
            output += d->printAppointments();
            output += "\n";
        }
        return output;
    }

    ~Database()
    {
        for (auto &d : days)
        {
            delete d;
        }
    }

    void saveToFile()
    {
        std::ofstream file(filename);
        for (const auto &d : days)
        {
            const auto &appts = d->getAppointments();
            for (const auto &apt : appts)
            {
                file << apt->toFileString() << "\n";
            }
        }
    }
};