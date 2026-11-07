#include "model/FollowUpPatient.h"
#include "utils/ClinicException.h"

namespace scapms {

FollowUpPatient::FollowUpPatient(int id, const std::string& name, int age,
                                 const std::string& contact, const std::string& previousVisitDate,
                                 const std::string& doctorName)
    : PatientRecord(id, name, age, contact),
      previousVisitDate_(previousVisitDate), doctorName_(doctorName)
{
    if (!Appointment::isValidDate(previousVisitDate))
        throw InvalidPatientException("Previous visit date must be in the format YYYY-MM-DD.");
    if (doctorName.empty())
        throw InvalidPatientException("Doctor name cannot be empty.");
}

std::string FollowUpPatient::category() const { return "Follow-Up"; }
int FollowUpPatient::priority() const { return 2; }

std::string FollowUpPatient::describe() const
{
    return "Follow-up with " + doctorName_ + " (last visit " + previousVisitDate_ + ")";
}

std::unique_ptr<PatientRecord> FollowUpPatient::clone() const
{
    return std::make_unique<FollowUpPatient>(*this);
}

void FollowUpPatient::scheduleAppointment(const Appointment& appt) { addAppointment(appt); }
bool FollowUpPatient::cancelAppointment(int appointmentId) { return cancelAppointmentById(appointmentId); }
int  FollowUpPatient::pendingAppointmentCount() const { return countByStatus(AppointmentStatus::Pending); }

const std::string& FollowUpPatient::previousVisitDate() const { return previousVisitDate_; }
const std::string& FollowUpPatient::doctorName() const { return doctorName_; }

void FollowUpPatient::setPreviousVisitDate(const std::string& date)
{
    if (!Appointment::isValidDate(date))
        throw InvalidPatientException("Previous visit date must be in the format YYYY-MM-DD.");
    previousVisitDate_ = date;
}

void FollowUpPatient::setDoctorName(const std::string& doctor)
{
    if (doctor.empty())
        throw InvalidPatientException("Doctor name cannot be empty.");
    doctorName_ = doctor;
}

} // namespace scapms