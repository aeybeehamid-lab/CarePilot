#include "model/Appointment.h"
#include "utils/ClinicException.h"
#include <cctype>

namespace carepilot {

static bool allDigits(const std::string& s, size_t from, size_t len)
{
    for (size_t i = from; i < from + len; ++i)
        if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
    return true;
}

bool Appointment::isValidDate(const std::string& d)
{
    if (d.size() != 10 || d[4] != '-' || d[7] != '-') return false;
    if (!allDigits(d, 0, 4) || !allDigits(d, 5, 2) || !allDigits(d, 8, 2)) return false;

    int year  = std::stoi(d.substr(0, 4));
    int month = std::stoi(d.substr(5, 2));
    int day   = std::stoi(d.substr(8, 2));
    if (month < 1 || month > 12 || day < 1) return false;

    static const int days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    int maxDay = days[month - 1];
    bool leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    if (month == 2 && leap) maxDay = 29;
    return day <= maxDay;
}

bool Appointment::isValidTime(const std::string& t)
{
    if (t.size() != 5 || t[2] != ':') return false;
    if (!allDigits(t, 0, 2) || !allDigits(t, 3, 2)) return false;
    int h = std::stoi(t.substr(0, 2));
    int m = std::stoi(t.substr(3, 2));
    return h < 24 && m < 60;
}

std::string Appointment::statusToText(AppointmentStatus s)
{
    switch (s) {
        case AppointmentStatus::Pending:   return "Pending";
        case AppointmentStatus::Completed: return "Completed";
        case AppointmentStatus::Cancelled: return "Cancelled";
    }
    return "Pending";
}

AppointmentStatus Appointment::statusFromText(const std::string& s)
{
    if (s == "Pending")   return AppointmentStatus::Pending;
    if (s == "Completed") return AppointmentStatus::Completed;
    if (s == "Cancelled") return AppointmentStatus::Cancelled;
    throw InvalidAppointmentException("Unknown appointment status: " + s);
}

Appointment::Appointment(int id, int patientId, const std::string& date,
                         const std::string& time, const std::string& reason,
                         AppointmentStatus status)
    : id_(id), patientId_(patientId), date_(date), time_(time),
      reason_(reason), status_(status)
{
    if (id <= 0)
        throw InvalidAppointmentException("Appointment ID must be a positive number.");
    if (!isValidDate(date))
        throw InvalidAppointmentException("Invalid date. Use the format YYYY-MM-DD.");
    if (!isValidTime(time))
        throw InvalidAppointmentException("Invalid time. Use the format HH:MM (24-hour).");
    if (reason.empty())
        throw InvalidAppointmentException("Appointment reason cannot be empty.");
}

int Appointment::id() const { return id_; }
int Appointment::patientId() const { return patientId_; }
const std::string& Appointment::date() const { return date_; }
const std::string& Appointment::time() const { return time_; }
const std::string& Appointment::reason() const { return reason_; }
AppointmentStatus Appointment::status() const { return status_; }
void Appointment::setStatus(AppointmentStatus s) { status_ = s; }

} // namespace carepilot