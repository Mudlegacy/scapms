#include "controller/ReminderService.h"

#include "utils/ClinicException.h"
#include "utils/StringUtils.h"
#include "utils/Validator.h"

namespace scapms {
namespace controller {

using model::Appointment;

std::time_t ReminderService::toTimestamp(const std::string& date, const std::string& timeText) {
    utils::Validator::validateDate(date);        // after these two checks the layout is certain
    utils::Validator::validateTime(timeText);

    int year = 0, month = 0, day = 0, hour = 0, minute = 0;
    utils::tryParseInt(date.substr(0, 4), year);
    utils::tryParseInt(date.substr(5, 2), month);
    utils::tryParseInt(date.substr(8, 2), day);
    utils::tryParseInt(timeText.substr(0, 2), hour);
    utils::tryParseInt(timeText.substr(3, 2), minute);

    std::tm parts = std::tm();
    parts.tm_year = year - 1900;     // years since 1900
    parts.tm_mon = month - 1;        // months are 0 to 11
    parts.tm_mday = day;
    parts.tm_hour = hour;
    parts.tm_min = minute;
    parts.tm_isdst = -1;             // let the computer decide about daylight saving
    return std::mktime(&parts);
}

std::vector<Reminder> ReminderService::findReminders(const std::vector<Appointment>& appointments,
                                                     std::time_t now, int windowMinutes) {
    if (windowMinutes < 0) {
        throw utils::InvalidDataException("Reminder window cannot be negative.");
    }

    std::vector<Reminder> found;
    std::vector<long> offsets;       // seconds until each reminder (negative = overdue)

    for (size_t i = 0; i < appointments.size(); i++) {
        const Appointment& appointment = appointments[i];
        if (!appointment.isPending()) continue;        // completed and cancelled ones are finished

        std::time_t when;
        try {
            when = toTimestamp(appointment.getDate(), appointment.getTime());
        } catch (const utils::ClinicException&) {
            continue;                                  // a corrupt record must not break the rest
        }

        long seconds = static_cast<long>(std::difftime(when, now));
        if (seconds > static_cast<long>(windowMinutes) * 60) continue;   // too far in the future

        Reminder reminder;
        reminder.appointmentId = appointment.getAppointmentId();
        reminder.patientId = appointment.getPatientId();
        reminder.date = appointment.getDate();
        reminder.time = appointment.getTime();
        reminder.reason = appointment.getReason();
        reminder.overdue = (seconds < 0);
        reminder.minutes = static_cast<int>((seconds < 0 ? -seconds : seconds) / 60);

        found.push_back(reminder);
        offsets.push_back(seconds);
    }

    // Our own insertion sort, soonest first. Both lists move together.
    for (size_t i = 1; i < found.size(); i++) {
        Reminder keyReminder = found[i];
        long keyOffset = offsets[i];
        size_t j = i;
        while (j > 0 && offsets[j - 1] > keyOffset) {
            found[j] = found[j - 1];
            offsets[j] = offsets[j - 1];
            j--;
        }
        found[j] = keyReminder;
        offsets[j] = keyOffset;
    }
    return found;
}

} // namespace controller
} // namespace scapms