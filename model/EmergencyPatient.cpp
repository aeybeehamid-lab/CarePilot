#include "model/EmergencyPatient.h"
#include "utils/ClinicException.h"

namespace scapms {

static void validateSeverity(int s)
{
    if (s < 1 || s > 5)
        throw InvalidPatientException("Severity must be between 1 and 5.");
}

EmergencyPatient::EmergencyPatient(int id, const std::string& name, int age,
                                   const std::string& contact, int severity,
                                   const std::string& triageNote)
    : PatientRecord(id, name, age, contact), severity_(severity), triageNote_(triageNote)
{
    validateSeverity(severity);
}

std::string EmergencyPatient::category() const { return "Emergency"; }
int EmergencyPatient::priority() const { return 1; }

std::string EmergencyPatient::describe() const
{
    std::string text = "Severity " + std::to_string(severity_) + "/5";
    if (isCritical()) text += " (CRITICAL)";
    if (!triageNote_.empty()) text += ": " + triageNote_;
    return text;
}

std::unique_ptr<PatientRecord> EmergencyPatient::clone() const
{
    return std::make_unique<EmergencyPatient>(*this);
}

int EmergencyPatient::severity() const { return severity_; }
const std::string& EmergencyPatient::triageNote() const { return triageNote_; }
bool EmergencyPatient::isCritical() const { return severity_ >= 4; }

void EmergencyPatient::setSeverity(int severity) { validateSeverity(severity); severity_ = severity; }
void EmergencyPatient::setTriageNote(const std::string& note) { triageNote_ = note; }

} // namespace scapms