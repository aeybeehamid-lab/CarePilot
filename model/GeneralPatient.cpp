#include "model/GeneralPatient.h"
#include "utils/ClinicException.h"

namespace carepilot {

GeneralPatient::GeneralPatient(int id, const std::string& name, int age,
                               const std::string& contact, const std::string& reasonForVisit)
    : PatientRecord(id, name, age, contact), reasonForVisit_(reasonForVisit)
{
    if (reasonForVisit.empty())
        throw InvalidPatientException("Reason for visit cannot be empty.");
}

std::string GeneralPatient::category() const { return "General"; }
int GeneralPatient::priority() const { return 3; }
std::string GeneralPatient::describe() const { return "Reason: " + reasonForVisit_; }

std::unique_ptr<PatientRecord> GeneralPatient::clone() const
{
    return std::make_unique<GeneralPatient>(*this);
}

void GeneralPatient::scheduleAppointment(const Appointment& appt) { addAppointment(appt); }
bool GeneralPatient::cancelAppointment(int appointmentId) { return cancelAppointmentById(appointmentId); }
int  GeneralPatient::pendingAppointmentCount() const { return countByStatus(AppointmentStatus::Pending); }

const std::string& GeneralPatient::reasonForVisit() const { return reasonForVisit_; }

void GeneralPatient::setReasonForVisit(const std::string& reason)
{
    if (reason.empty())
        throw InvalidPatientException("Reason for visit cannot be empty.");
    reasonForVisit_ = reason;
}

} // namespace carepilot