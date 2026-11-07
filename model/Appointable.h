#pragma once
#include "model/Appointment.h"

namespace carepilot {

class Appointable {
public:
    virtual ~Appointable() = default;
    virtual void scheduleAppointment(const Appointment& appt) = 0;
    virtual bool cancelAppointment(int appointmentId) = 0;
    virtual int  pendingAppointmentCount() const = 0;
};

} // namespace carepilot