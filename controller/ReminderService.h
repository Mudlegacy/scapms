#ifndef REMINDERSERVICE_H
#define REMINDERSERVICE_H

#include <ctime>
#include <string>
#include <vector>

#include "model/Appointment.h"

namespace scapms {
namespace controller {

// One appointment that needs attention
struct Reminder {
    int appointmentId = 0;
    int patientId = 0;
    std::string date;
    std::string time;
    std::string reason;
    bool overdue = false;     // true if the start time has already passed
    int minutes = 0;          // overdue: minutes late. Otherwise: minutes until it starts
};

// Contains no Qt code. The current time is passed IN, so the logic
// can be tested with any "now" we like.
class ReminderService {
public:
    // Converts a date (YYYY-MM-DD) and time (HH:MM) into a point in time
    static std::time_t toTimestamp(const std::string& date, const std::string& timeText);

    // Pending appointments that start within windowMinutes of 'now',
    // plus pending ones whose start time has already passed.
    // The result is sorted soonest first (the most overdue come first).
    static std::vector<Reminder> findReminders(const std::vector<model::Appointment>& appointments,
                                               std::time_t now, int windowMinutes);
};

} // namespace controller
} // namespace scapms

#endif