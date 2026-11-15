#include "model/Appointment.h"

namespace scapms {
namespace model {

std::string statusToString(AppointmentStatus status) {
    switch (status) {
        case AppointmentStatus::Pending:   return "Pending";
        case AppointmentStatus::Completed: return "Completed";
        case AppointmentStatus::Cancelled: return "Cancelled";
    }
    return "Unknown";
}

Appointment::Appointment(int appointmentId, int patientId, const std::string& date,
                         const std::string& time, const std::string& reason)
    : appointmentId(appointmentId), patientId(patientId), date(date),
      time(time), reason(reason), status(AppointmentStatus::Pending) {}

int Appointment::getAppointmentId() const { return appointmentId; }
int Appointment::getPatientId() const { return patientId; }
const std::string& Appointment::getDate() const { return date; }
const std::string& Appointment::getTime() const { return time; }
const std::string& Appointment::getReason() const { return reason; }
AppointmentStatus Appointment::getStatus() const { return status; }

void Appointment::setDate(const std::string& newDate) { date = newDate; }
void Appointment::setTime(const std::string& newTime) { time = newTime; }
void Appointment::setReason(const std::string& newReason) { reason = newReason; }

void Appointment::markCompleted() { status = AppointmentStatus::Completed; }
void Appointment::cancel() { status = AppointmentStatus::Cancelled; }
void Appointment::reopen() { status = AppointmentStatus::Pending; }
bool Appointment::isPending() const { return status == AppointmentStatus::Pending; }

std::string Appointment::toString() const {
    return "Appointment #" + std::to_string(appointmentId) +
           " | Patient ID: " + std::to_string(patientId) +
           " | " + date + " " + time +
           " | Reason: " + reason +
           " | Status: " + statusToString(status);
}

} // namespace model
} // namespace scapms