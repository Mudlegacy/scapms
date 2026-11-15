#ifndef APPOINTABLE_H
#define APPOINTABLE_H

#include <vector>
#include "model/Appointment.h"

namespace scapms {
namespace model {

// Interface: only pure virtual functions, no data.
// Any patient type that can be scheduled must implement these.
class Appointable {
public:
    virtual ~Appointable() = default;

    virtual void scheduleAppointment(const Appointment& appointment) = 0;
    virtual bool cancelAppointment(int appointmentId) = 0;
    virtual bool completeAppointment(int appointmentId) = 0;
    virtual bool reopenAppointment(int appointmentId) = 0;
    virtual int countPendingAppointments() const = 0;
    virtual const std::vector<Appointment>& getAppointments() const = 0;
};

} // namespace model
} // namespace scapms

#endif