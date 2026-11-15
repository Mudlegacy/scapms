#include "model/AppointmentBook.h"

namespace scapms {
namespace model {

void AppointmentBook::add(const Appointment& appointment) {
    items.push_back(appointment);
}

bool AppointmentBook::cancel(int appointmentId) {
    for (size_t i = 0; i < items.size(); i++) {
        if (items[i].getAppointmentId() == appointmentId) {
            items[i].cancel();       // keep the record, just change its status
            return true;
        }
    }
    return false;
}

bool AppointmentBook::complete(int appointmentId) {
    for (size_t i = 0; i < items.size(); i++) {
        if (items[i].getAppointmentId() == appointmentId) {
            items[i].markCompleted();
            return true;
        }
    }
    return false;
}

bool AppointmentBook::reopen(int appointmentId) {
    for (size_t i = 0; i < items.size(); i++) {
        if (items[i].getAppointmentId() == appointmentId) {
            items[i].reopen();
            return true;
        }
    }
    return false;
}

int AppointmentBook::countPending() const {
    int count = 0;
    for (size_t i = 0; i < items.size(); i++) {
        if (items[i].isPending()) count++;
    }
    return count;
}

const std::vector<Appointment>& AppointmentBook::getAll() const {
    return items;
}

} // namespace model
} // namespace scapms