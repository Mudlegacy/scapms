#ifndef APPOINTMENTBOOK_H
#define APPOINTMENTBOOK_H

#include <vector>
#include "model/Appointment.h"

namespace scapms {
namespace model {

// Holds all appointments belonging to one patient.
class AppointmentBook {
private:
    std::vector<Appointment> items;

public:
    void add(const Appointment& appointment);
    bool cancel(int appointmentId);          // true if found
    bool complete(int appointmentId);        // true if found
    bool reopen(int appointmentId);          // true if found
    int countPending() const;
    const std::vector<Appointment>& getAll() const;
};

} // namespace model
} // namespace scapms

#endif