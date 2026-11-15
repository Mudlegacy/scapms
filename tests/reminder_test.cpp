#include <ctime>
#include <iostream>
#include <string>
#include <vector>

#include "controller/ReminderService.h"
#include "model/Appointment.h"
#include "utils/ClinicException.h"

using namespace scapms::controller;
using namespace scapms::model;

void show(int window, const std::vector<Reminder>& reminders) {
    std::cout << "Window " << window << " minutes (" << reminders.size() << ")\n";
    for (size_t i = 0; i < reminders.size(); i++) {
        const Reminder& r = reminders[i];
        std::cout << "  #" << r.appointmentId << " " << r.date << " " << r.time << " - "
                  << (r.overdue ? "OVERDUE by " : "in ") << r.minutes << " min - "
                  << r.reason << "\n";
    }
    std::cout << "\n";
}

int main() {
    // A fixed "now", so the answers never change
    std::time_t now = ReminderService::toTimestamp("2026-11-12", "10:00");

    std::vector<Appointment> list;
    list.push_back(Appointment(1, 1, "2026-11-12", "10:15", "In 15 minutes"));
    list.push_back(Appointment(2, 1, "2026-11-12", "10:45", "In 45 minutes"));
    list.push_back(Appointment(3, 2, "2026-11-12", "09:50", "10 minutes late"));
    list.push_back(Appointment(4, 2, "2026-11-02", "08:00", "Ten days late"));
    list.push_back(Appointment(5, 3, "2026-11-12", "10:10", "Cancelled one"));
    list.back().cancel();
    list.push_back(Appointment(6, 3, "2026-11-12", "10:05", "Completed one"));
    list.back().markCompleted();
    list.push_back(Appointment(7, 4, "2026-11-12", "10:30", "Exactly 30 minutes"));
    list.push_back(Appointment(8, 4, "2026-11-12", "10:00", "Right now"));
    list.push_back(Appointment(9, 4, "2026-11-12", "99:99", "Corrupt time"));

    int windows[] = {30, 60, 5, 0};
    for (int i = 0; i < 4; i++) {
        show(windows[i], ReminderService::findReminders(list, now, windows[i]));
    }

    try {
        ReminderService::findReminders(list, now, -5);
    } catch (const scapms::utils::ClinicException& e) {
        std::cout << "Rejected: " << e.what() << "\n";
    }
    return 0;
}