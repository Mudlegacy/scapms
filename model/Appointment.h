#ifndef APPOINTMENT_H
#define APPOINTMENT_H

#include <string>

namespace scapms {
namespace model {

enum class AppointmentStatus { Pending, Completed, Cancelled };

std::string statusToString(AppointmentStatus status);

class Appointment {
private:
    int appointmentId;
    int patientId;            // which patient this belongs to
    std::string date;         // format: YYYY-MM-DD
    std::string time;         // format: HH:MM
    std::string reason;
    AppointmentStatus status;

public:
    Appointment(int appointmentId, int patientId, const std::string& date,
                const std::string& time, const std::string& reason);

    int getAppointmentId() const;
    int getPatientId() const;
    const std::string& getDate() const;
    const std::string& getTime() const;
    const std::string& getReason() const;
    AppointmentStatus getStatus() const;

    void setDate(const std::string& newDate);
    void setTime(const std::string& newTime);
    void setReason(const std::string& newReason);

    void markCompleted();
    void cancel();
    void reopen();            // sets the status back to Pending (used by Undo)
    bool isPending() const;

    std::string toString() const;
};

} // namespace model
} // namespace scapms

#endif