#include "model/PatientRecord.h"
#include "utils/ClinicException.h"
#include <cctype>

namespace carepilot {

static bool isBlank(const std::string& s)
{
    for (char c : s)
        if (!std::isspace(static_cast<unsigned char>(c))) return false;
    return true;
}

void PatientRecord::validateName(const std::string& name)
{
    if (isBlank(name))
        throw InvalidPatientException("Patient name cannot be empty.");
}

void PatientRecord::validateAge(int age)
{
    if (age < 0 || age > 120)
        throw InvalidPatientException("Patient age must be between 0 and 120.");
}

void PatientRecord::validateContact(const std::string& contact)
{
    if (isBlank(contact))
        throw InvalidPatientException("Contact information cannot be empty.");
}

PatientRecord::PatientRecord(int id, const std::string& name, int age,
                             const std::string& contact)
    : id_(id), name_(name), age_(age), contact_(contact)
{
    if (id <= 0)
        throw InvalidPatientException("Patient ID must be a positive number.");
    validateName(name);
    validateAge(age);
    validateContact(contact);
}

int PatientRecord::id() const { return id_; }
const std::string& PatientRecord::name() const { return name_; }
int PatientRecord::age() const { return age_; }
const std::string& PatientRecord::contact() const { return contact_; }

void PatientRecord::setName(const std::string& name)       { validateName(name);       name_ = name; }
void PatientRecord::setAge(int age)                        { validateAge(age);         age_ = age; }
void PatientRecord::setContact(const std::string& contact) { validateContact(contact); contact_ = contact; }

const std::vector<Appointment>& PatientRecord::appointments() const { return appointments_; }

int PatientRecord::countByStatus(AppointmentStatus s) const
{
    int count = 0;
    for (const Appointment& a : appointments_)
        if (a.status() == s) ++count;
    return count;
}

void PatientRecord::addAppointment(const Appointment& appt)
{
    if (appt.patientId() != id_)
        throw InvalidAppointmentException("Appointment belongs to a different patient.");

    for (const Appointment& a : appointments_) {
        if (a.id() == appt.id())
            throw InvalidAppointmentException("Appointment ID already exists.");
        if (a.status() == AppointmentStatus::Pending &&
            a.date() == appt.date() && a.time() == appt.time())
            throw InvalidAppointmentException("Patient already has an appointment at that date and time.");
    }
    appointments_.push_back(appt);
}

bool PatientRecord::cancelAppointmentById(int appointmentId)
{
    for (Appointment& a : appointments_) {
        if (a.id() == appointmentId && a.status() == AppointmentStatus::Pending) {
            a.setStatus(AppointmentStatus::Cancelled);
            return true;
        }
    }
    return false;
}

bool PatientRecord::completeAppointment(int appointmentId)
{
    for (Appointment& a : appointments_) {
        if (a.id() == appointmentId && a.status() == AppointmentStatus::Pending) {
            a.setStatus(AppointmentStatus::Completed);
            return true;
        }
    }
    return false;
}

} // namespace carepilot