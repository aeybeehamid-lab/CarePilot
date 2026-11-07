#pragma once
#include "model/PatientRecord.h"
#include "model/Appointable.h"

namespace scapms {

class GeneralPatient : public PatientRecord, public Appointable {
public:
    GeneralPatient(int id, const std::string& name, int age,
                   const std::string& contact, const std::string& reasonForVisit);

    // PatientRecord
    std::string category() const override;
    int priority() const override;
    std::string describe() const override;
    std::unique_ptr<PatientRecord> clone() const override;

    // Appointable
    void scheduleAppointment(const Appointment& appt) override;
    bool cancelAppointment(int appointmentId) override;
    int  pendingAppointmentCount() const override;

    const std::string& reasonForVisit() const;
    void setReasonForVisit(const std::string& reason);

private:
    std::string reasonForVisit_;
};

} // namespace scapms