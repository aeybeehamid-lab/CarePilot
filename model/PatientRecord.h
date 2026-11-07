#pragma once
#include "model/Appointment.h"
#include <memory>
#include <string>
#include <vector>

namespace carepilot {

class PatientRecord {
public:
    PatientRecord(int id, const std::string& name, int age, const std::string& contact);
    virtual ~PatientRecord() = default;

    // --- Pure virtual: every patient type must define these ---
    virtual std::string category() const = 0;                  // "General", "Emergency", ...
    virtual int priority() const = 0;                          // 1 = highest priority
    virtual std::string describe() const = 0;                  // type-specific summary
    virtual std::unique_ptr<PatientRecord> clone() const = 0;  // used by Undo

    // --- Common data (validated) ---
    int id() const;
    const std::string& name() const;
    int age() const;
    const std::string& contact() const;
    void setName(const std::string& name);
    void setAge(int age);
    void setContact(const std::string& contact);

    // --- Appointments (composition: a patient owns its appointments) ---
    const std::vector<Appointment>& appointments() const;
    int  countByStatus(AppointmentStatus s) const;
    bool completeAppointment(int appointmentId);

    static void validateName(const std::string& name);
    static void validateAge(int age);
    static void validateContact(const std::string& contact);

protected:
    // Subclasses that implement Appointable use these helpers.
    void addAppointment(const Appointment& appt);
    bool cancelAppointmentById(int appointmentId);

private:
    int id_;
    std::string name_;
    int age_;
    std::string contact_;
    std::vector<Appointment> appointments_;
};

} // namespace carepilot