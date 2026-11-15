#include "controller/ReportGenerator.h"

#include <vector>

#include "model/Appointable.h"
#include "utils/ClinicException.h"
#include "utils/StringUtils.h"
#include "utils/Validator.h"

namespace scapms {
namespace controller {

using model::Appointable;
using model::Appointment;
using model::AppointmentStatus;
using model::ClinicDatabase;
using model::PatientRecord;

namespace {

std::string padRight(const std::string& text, size_t width) {
    std::string result = text;
    while (result.size() < width) result += ' ';
    return result;
}

std::string patientLabel(const PatientRecord* p) {
    return p->getName() + " (ID " + std::to_string(p->getId()) + ")";
}

int countWithStatus(const Appointable* a, AppointmentStatus status) {
    int count = 0;
    const std::vector<Appointment>& list = a->getAppointments();
    for (size_t i = 0; i < list.size(); i++) {
        if (list[i].getStatus() == status) count++;
    }
    return count;
}

struct AttendanceRow {
    const PatientRecord* patient;
    int completed;
};

// Most completed first. On a tie, alphabetical by name.
bool comesBefore(const AttendanceRow& a, const AttendanceRow& b) {
    if (a.completed != b.completed) return a.completed > b.completed;
    return utils::compareIgnoreCase(a.patient->getName(), b.patient->getName()) < 0;
}

// Our own insertion sort (the report list is small)
void insertionSortRows(std::vector<AttendanceRow>& rows) {
    for (size_t i = 1; i < rows.size(); i++) {
        AttendanceRow key = rows[i];
        size_t j = i;
        while (j > 0 && comesBefore(key, rows[j - 1])) {
            rows[j] = rows[j - 1];
            j--;
        }
        rows[j] = key;
    }
}

} // anonymous namespace

// ---------------- Report 1 ----------------

std::string ReportGenerator::mostFrequentlyAttended(const ClinicDatabase& db, int topN) {
    if (topN < 1) {
        throw utils::InvalidDataException("Number of patients to show must be at least 1.");
    }

    std::vector<AttendanceRow> rows;
    const std::vector<PatientRecord*>& patients = db.getAll();

    for (size_t i = 0; i < patients.size(); i++) {
        const Appointable* a = dynamic_cast<const Appointable*>(patients[i]);
        if (a == nullptr) continue;             // this patient type has no appointments

        int completed = countWithStatus(a, AppointmentStatus::Completed);
        if (completed > 0) {
            AttendanceRow row;
            row.patient = patients[i];
            row.completed = completed;
            rows.push_back(row);
        }
    }

    insertionSortRows(rows);

    std::string report = "MOST FREQUENTLY ATTENDED PATIENTS (top " + std::to_string(topN) + ")\n";
    if (rows.empty()) {
        report += "  No completed appointments yet.\n";
        return report;
    }
    for (size_t i = 0; i < rows.size() && i < static_cast<size_t>(topN); i++) {
        report += "  " + std::to_string(i + 1) + ". " + patientLabel(rows[i].patient) +
                  " - " + std::to_string(rows[i].completed) + " completed\n";
    }
    return report;
}

// ---------------- Report 2 ----------------

std::string ReportGenerator::patientsWithPending(const ClinicDatabase& db) {
    std::string report = "PATIENTS WITH PENDING APPOINTMENTS\n";
    int found = 0;
    const std::vector<PatientRecord*>& patients = db.getAll();

    for (size_t i = 0; i < patients.size(); i++) {
        const Appointable* a = dynamic_cast<const Appointable*>(patients[i]);
        if (a == nullptr) continue;

        int pending = a->countPendingAppointments();
        if (pending == 0) continue;

        found++;
        report += "  " + patientLabel(patients[i]) + " - " + std::to_string(pending) + " pending\n";

        const std::vector<Appointment>& list = a->getAppointments();
        for (size_t j = 0; j < list.size(); j++) {
            if (list[j].isPending()) {
                report += "      #" + std::to_string(list[j].getAppointmentId()) + " " +
                          list[j].getDate() + " " + list[j].getTime() + " - " +
                          list[j].getReason() + "\n";
            }
        }
    }

    if (found == 0) {
        report += "  No pending appointments.\n";
    }
    return report;
}

// ---------------- Report 3 ----------------

std::string ReportGenerator::categoryDistribution(const ClinicDatabase& db) {
    int general = 0;
    int emergency = 0;
    int followUp = 0;

    const std::vector<PatientRecord*>& patients = db.getAll();
    for (size_t i = 0; i < patients.size(); i++) {
        std::string category = patients[i]->getCategory();
        if (category == "General")        general++;
        else if (category == "Emergency") emergency++;
        else if (category == "Follow-Up") followUp++;
    }

    const size_t WIDTH = 20;
    std::string report = "PATIENT CATEGORY DISTRIBUTION\n";
    report += "  " + padRight("General Patients", WIDTH)   + std::to_string(general)   + "\n";
    report += "  " + padRight("Emergency Patients", WIDTH) + std::to_string(emergency) + "\n";
    report += "  " + padRight("Follow-Up Patients", WIDTH) + std::to_string(followUp)  + "\n";
    report += "  " + padRight("Total", WIDTH)              + std::to_string(db.size()) + "\n";
    return report;
}

// ---------------- Report 4 ----------------

std::string ReportGenerator::dailySummary(const ClinicDatabase& db, const std::string& date) {
    utils::Validator::validateDate(date);    // bad input is rejected, not guessed at

    int total = 0;
    int completed = 0;
    int pending = 0;
    int cancelled = 0;

    const std::vector<PatientRecord*>& patients = db.getAll();
    for (size_t i = 0; i < patients.size(); i++) {
        const Appointable* a = dynamic_cast<const Appointable*>(patients[i]);
        if (a == nullptr) continue;

        const std::vector<Appointment>& list = a->getAppointments();
        for (size_t j = 0; j < list.size(); j++) {
            if (list[j].getDate() != date) continue;
            total++;
            switch (list[j].getStatus()) {
                case AppointmentStatus::Completed: completed++; break;
                case AppointmentStatus::Pending:   pending++;   break;
                case AppointmentStatus::Cancelled: cancelled++; break;
            }
        }
    }

    std::string report = "DAILY APPOINTMENT SUMMARY - " + date + "\n";
    report += "  Total Appointments: " + std::to_string(total) + "\n";
    report += "  Completed: " + std::to_string(completed) + "\n";
    report += "  Pending: " + std::to_string(pending) + "\n";
    report += "  Cancelled: " + std::to_string(cancelled) + "\n";
    return report;
}

} // namespace controller
} // namespace scapms