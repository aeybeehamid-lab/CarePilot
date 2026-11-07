#pragma once
#include "model/PatientRecord.h"

namespace carepilot {

class EmergencyPatient : public PatientRecord {
public:
    EmergencyPatient(int id, const std::string& name, int age,
                     const std::string& contact, int severity,
                     const std::string& triageNote);

    std::string category() const override;
    int priority() const override;
    std::string describe() const override;
    std::unique_ptr<PatientRecord> clone() const override;

    int severity() const;                    // 1 (mild) to 5 (life-threatening)
    const std::string& triageNote() const;
    bool isCritical() const;                 // severity >= 4
    void setSeverity(int severity);
    void setTriageNote(const std::string& note);

private:
    int severity_;
    std::string triageNote_;
};

} // namespace carepilot