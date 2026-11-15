#include "model/FollowUpPatient.h"

namespace scapms {
namespace model {

FollowUpPatient::FollowUpPatient(int id, const std::string& name, int age,
                                 const std::string& contact,
                                 const std::string& previousDiagnosis,
                                 const std::string& lastVisitDate)
    : PatientRecord(id, name, age, contact),
      previousDiagnosis(previousDiagnosis), lastVisitDate(lastVisitDate) {}

const std::string& FollowUpPatient::getPreviousDiagnosis() const { return previousDiagnosis; }
const std::string& FollowUpPatient::getLastVisitDate() const { return lastVisitDate; }
void FollowUpPatient::setPreviousDiagnosis(const std::string& diagnosis) { previousDiagnosis = diagnosis; }
void FollowUpPatient::setLastVisitDate(const std::string& date) { lastVisitDate = date; }

std::string FollowUpPatient::getCategory() const { return "Follow-Up"; }

// Scheduled visits can wait behind the other two types
int FollowUpPatient::getPriority() const { return 3; }

std::string FollowUpPatient::getSummary() const {
    return "[Follow-Up] ID: " + std::to_string(getId()) +
           " | Name: " + getName() +
           " | Age: " + std::to_string(getAge()) +
           " | Contact: " + getContact() +
           " | Previous Diagnosis: " + previousDiagnosis +
           " | Last Visit: " + lastVisitDate;
}

// ---- Appointable interface ----

void FollowUpPatient::scheduleAppointment(const Appointment& appointment) {
    appointments.add(appointment);
}

bool FollowUpPatient::cancelAppointment(int appointmentId) {
    return appointments.cancel(appointmentId);
}

bool FollowUpPatient::completeAppointment(int appointmentId) {
    return appointments.complete(appointmentId);
}

bool FollowUpPatient::reopenAppointment(int appointmentId) {
    return appointments.reopen(appointmentId);
}

int FollowUpPatient::countPendingAppointments() const {
    return appointments.countPending();
}

const std::vector<Appointment>& FollowUpPatient::getAppointments() const {
    return appointments.getAll();
}

} // namespace model
} // namespace scapms