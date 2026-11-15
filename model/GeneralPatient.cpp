#include "model/GeneralPatient.h"

namespace scapms {
namespace model {

GeneralPatient::GeneralPatient(int id, const std::string& name, int age,
                               const std::string& contact, const std::string& complaint)
    : PatientRecord(id, name, age, contact), complaint(complaint) {}

const std::string& GeneralPatient::getComplaint() const { return complaint; }
void GeneralPatient::setComplaint(const std::string& newComplaint) { complaint = newComplaint; }

std::string GeneralPatient::getCategory() const { return "General"; }

// Lower number = more urgent. Emergency is 1, General 2, Follow-Up 3.
int GeneralPatient::getPriority() const { return 2; }

std::string GeneralPatient::getSummary() const {
    return "[General] ID: " + std::to_string(getId()) +
           " | Name: " + getName() +
           " | Age: " + std::to_string(getAge()) +
           " | Contact: " + getContact() +
           " | Complaint: " + complaint;
}

// ---- Appointable interface ----

void GeneralPatient::scheduleAppointment(const Appointment& appointment) {
    appointments.add(appointment);
}

bool GeneralPatient::cancelAppointment(int appointmentId) {
    return appointments.cancel(appointmentId);
}

bool GeneralPatient::completeAppointment(int appointmentId) {
    return appointments.complete(appointmentId);
}

bool GeneralPatient::reopenAppointment(int appointmentId) {
    return appointments.reopen(appointmentId);
}

int GeneralPatient::countPendingAppointments() const {
    return appointments.countPending();
}

const std::vector<Appointment>& GeneralPatient::getAppointments() const {
    return appointments.getAll();
}

} // namespace model
} // namespace scapms