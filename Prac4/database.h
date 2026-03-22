#ifndef DATABASE_H
#define DATABASE_H 

 #include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iterator>
#include <regex>
#include <sys/stat.h>
#include <unistd.h>

static const std::string b64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

//Return the new path of the temp file.
static inline std::string writeTempFile(const std::string &bytes, const std::string &suffix = "") {
        char tmpl[] = "/tmp/imgXXXXXX";
        int fd = mkstemp(tmpl);
        if (fd == -1) return {};
        ssize_t w = write(fd, bytes.data(), bytes.size()); (void)w;
        close(fd);
        std::string path(tmpl);
        if (!suffix.empty()) {
                std::string newPath = path + suffix;
                rename(path.c_str(), newPath.c_str());
                path = newPath;
        }
        return path;
}

static inline std::string base64_encode(const std::string &in) {
        std::string out;
        int val = 0, valb = -6;
        for (unsigned char c : in) {
                val = (val << 8) + c;
                valb += 8;
                while (valb >= 0) {
                        out.push_back(b64_chars[(val >> valb) & 0x3F]);
                        valb -= 6;
                }
        }
        if (valb > -6) out.push_back(b64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
        while (out.size() % 4) out.push_back('=');
        return out;
}

static inline std::string base64_decode(const std::string &in) {
        std::string out;
        std::vector<int> T(256, -1);
        for (int i = 0; i < 64; ++i) T[(unsigned)b64_chars[i]] = i;
        int val = 0, valb = -8;
        for (unsigned char c : in) {
                if (T[c] == -1) break;
                val = (val << 6) + T[c];
                valb += 6;
                if (valb >= 0) {
                        out.push_back(char((val >> valb) & 0xFF));
                        valb -= 8;
                }
        }
        return out;
}

class Appointment
{
private:
    std::string date; // dd/mm/yyyy
    std::string time; // 24 hr format with hh//mm
    std::string contactee;
    std::string location;    // Should be able to be ignored
    std::string description; // Should be able to be ignored
    std::string imagePath;    // Should be able to be ignored
public:
    Appointment(std::string date, std::string time, std::string contactee,
                std::string location = "", std::string description = "", std::string imagePath = "")
        : date(date), time(time), contactee(contactee),
          location(location), description(description), imagePath(base64_decode(imagePath)) {}
    Appointment() : date(""), time(""), contactee(""), location(""), description(""), imagePath("") {}
    Appointment(std::string fileString)
    {
        size_t dStart = fileString.find("<D>") + 3;
        size_t dEnd = fileString.find("<D>", dStart);
        date = fileString.substr(dStart, dEnd - dStart);

        size_t tStart = fileString.find("<T>") + 3;
        size_t tEnd = fileString.find("<T>", tStart);
        time = fileString.substr(tStart, tEnd - tStart);

        size_t cStart = fileString.find("<C>") + 3;
        size_t cEnd = fileString.find("<C>", cStart);
        contactee = fileString.substr(cStart, cEnd - cStart);

        size_t lStart = fileString.find("<L>") + 3;
        size_t lEnd = fileString.find("<L>", lStart);
        location = fileString.substr(lStart, lEnd - lStart);

        size_t descStart = fileString.find("<DESC>") + 6;
        size_t descEnd = fileString.find("<DESC>", descStart);
        description = fileString.substr(descStart, descEnd - descStart);

        size_t imgStart = fileString.find("<IMG>") + 5;
        size_t imgEnd = fileString.find("<IMG>", imgStart);
        imagePath = fileString.substr(imgStart, imgEnd - imgStart);
    }
    ~Appointment() {}
    std::string getDate() const { return date; }
    std::string getTime() const { return time; }
    std::string getContactee() const { return contactee; }
    std::string getLocation() const { return location; }
    std::string getDescription() const { return description; }
    std::string getImagePath() const { //this return a HTML img tag using a temp file created from the base64 string stored in imagePath
        
        if (imagePath.empty()) return std::string();
        std::string bytes = base64_decode(imagePath);
        if (bytes.empty()) return std::string();
        std::string tmp = writeTempFile(bytes);
        if (tmp.empty()) return std::string();
        return std::string("<img src=\"") + tmp + "\" alt=\"image\">";
     }
    std::string toString() const
    {
        return "Date: " + date + "  Time: " + time + " Person: " + contactee +
               (location.empty() ? "" : " Location " + location) +
               (description.empty() ? "\n" : "\n Description: " + description);
    }
    bool search(const std::string &keyword) const
    {
        return date.find(keyword) != std::string::npos ||
               time.find(keyword) != std::string::npos ||
               contactee.find(keyword) != std::string::npos ||
               location.find(keyword) != std::string::npos ||
               description.find(keyword) != std::string::npos;
    } // Helper function for precise searching

    std::string toFileString() const //This is to write to the storage file, it should be the same format as the constructor that takes a file string, so they can read/write in the same format.
    {
        std::string out = "<D>" + date + "<D><T>" + time + "<T><C>" + contactee + "<C><L>" + location + "<L><DESC>" + description + "<DESC><IMG>"+imagePath+"<IMG>";
        return out;
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
                appointments[i]->getTime() == apt.getTime() &&
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

    void deleteAppointment(const std::string &date, const std::string &time = "", const std::string &contactee = "")
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
                if (!time.empty())
                {
                    for (const auto &apt : searchResults)
                    {
                        if (apt->search(time) == true)
                        {
                            ResultDate->deleteAppointment(ResultDate->getIndex(*apt));
                            return;
                        }
                    }
                }
                else
                {
                    for (const auto &apt : searchResults)
                    {
                        ResultDate->deleteAppointment(ResultDate->getIndex(*apt));
                    }
                }
            }
            else
            {
                if (!time.empty())
                {
                    for (const auto &apt : searchResults)
                    {
                        if (apt->search(time) == true)
                        {
                            ResultDate->deleteAppointment(ResultDate->getIndex(*apt));
                            return;
                        }
                    }
                }
                else
                {
                    for (const auto &apt : searchResults)
                    {
                        ResultDate->deleteAppointment(ResultDate->getIndex(*apt));
                    }
                }
            }
        }
        return;
    }

    const std::vector<day*> &getDays() const { return days; };

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

#endif //DATABASE_H