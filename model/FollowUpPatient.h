#pragma once
#include "model/PatientRecord.h"
#include "model/Appointable.h"

namespace scapms {

class FollowUpPatient : public PatientRecord, public Appointable {
public:
    FollowUpPatient(int id, const std::string& name, int age,
                    const std::string& contact, const std::string& previousVisitDate,
                    const std::string& doctorName);

    std::string category() const override;
    int priority() const override;
    std::string describe() const override;
    std::unique_ptr<PatientRecord> clone() const override;

    void scheduleAppointment(const Appointment& appt) override;
    bool cancelAppointment(int appointmentId) override;
    int  pendingAppointmentCount() const override;

    const std::string& previousVisitDate() const;
    const std::string& doctorName() const;
    void setPreviousVisitDate(const std::string& date);
    void setDoctorName(const std::string& doctor);

private:
    std::string previousVisitDate_;
    std::string doctorName_;
};

} // namespace scapms