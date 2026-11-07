#pragma once
#include <string>

namespace scapms {

enum class AppointmentStatus { Pending, Completed, Cancelled };

class Appointment {
public:
    Appointment(int id, int patientId, const std::string& date,
                const std::string& time, const std::string& reason,
                AppointmentStatus status = AppointmentStatus::Pending);

    int id() const;
    int patientId() const;
    const std::string& date() const;    // "YYYY-MM-DD"
    const std::string& time() const;    // "HH:MM"
    const std::string& reason() const;
    AppointmentStatus status() const;
    void setStatus(AppointmentStatus s);

    static bool isValidDate(const std::string& d);
    static bool isValidTime(const std::string& t);
    static std::string statusToText(AppointmentStatus s);
    static AppointmentStatus statusFromText(const std::string& s);

private:
    int id_;
    int patientId_;
    std::string date_;
    std::string time_;
    std::string reason_;
    AppointmentStatus status_;
};

} // namespace scapms