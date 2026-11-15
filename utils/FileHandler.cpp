#include "utils/FileHandler.h"

#include <fstream>
#include <vector>

#include "model/Appointable.h"
#include "model/EmergencyPatient.h"
#include "model/FollowUpPatient.h"
#include "model/GeneralPatient.h"
#include "utils/ClinicException.h"
#include "utils/IDGenerator.h"
#include "utils/StringUtils.h"
#include "utils/Validator.h"

namespace scapms {
namespace utils {

using model::Appointable;
using model::Appointment;
using model::ClinicDatabase;
using model::EmergencyPatient;
using model::FollowUpPatient;
using model::GeneralPatient;
using model::PatientRecord;
using model::statusToString;

// ============================== SAVE ==============================

void FileHandler::save(const ClinicDatabase& db, const std::string& path) {
    std::ofstream out(path);
    if (!out) {
        throw FileException("Cannot open file for writing: " + path);
    }

    out << "SCAPMS,1\n";   // header: identifies our file and its format version

    const std::vector<PatientRecord*>& patients = db.getAll();

    // 1. All patients first, so appointments can refer to them when loading
    for (size_t i = 0; i < patients.size(); i++) {
        const PatientRecord* p = patients[i];
        std::string common = std::to_string(p->getId()) + "," +
                             escapeField(p->getName()) + "," +
                             std::to_string(p->getAge()) + "," +
                             escapeField(p->getContact());

        if (const GeneralPatient* g = dynamic_cast<const GeneralPatient*>(p)) {
            out << "P,General," << common << ","
                << escapeField(g->getComplaint()) << "\n";
        } else if (const EmergencyPatient* e = dynamic_cast<const EmergencyPatient*>(p)) {
            out << "P,Emergency," << common << ","
                << e->getSeverityLevel() << ","
                << escapeField(e->getCondition()) << "\n";
        } else if (const FollowUpPatient* f = dynamic_cast<const FollowUpPatient*>(p)) {
            out << "P,FollowUp," << common << ","
                << escapeField(f->getPreviousDiagnosis()) << ","
                << escapeField(f->getLastVisitDate()) << "\n";
        }
    }

    // 2. Then every appointment of every patient who can have them
    for (size_t i = 0; i < patients.size(); i++) {
        const Appointable* a = dynamic_cast<const Appointable*>(patients[i]);
        if (a == nullptr) continue;

        const std::vector<Appointment>& list = a->getAppointments();
        for (size_t j = 0; j < list.size(); j++) {
            const Appointment& ap = list[j];
            out << "A," << ap.getAppointmentId() << ","
                << ap.getPatientId() << ","
                << escapeField(ap.getDate()) << ","
                << escapeField(ap.getTime()) << ","
                << escapeField(ap.getReason()) << ","
                << statusToString(ap.getStatus()) << "\n";
        }
    }

    out.close();
    if (out.fail()) {
        throw FileException("Failed while writing file: " + path);
    }
}

// ============================== LOAD ==============================

namespace {

bool appointmentIdInUse(const ClinicDatabase& db, int appointmentId) {
    const std::vector<PatientRecord*>& patients = db.getAll();
    for (size_t i = 0; i < patients.size(); i++) {
        const Appointable* a = dynamic_cast<const Appointable*>(patients[i]);
        if (a == nullptr) continue;
        const std::vector<Appointment>& list = a->getAppointments();
        for (size_t j = 0; j < list.size(); j++) {
            if (list[j].getAppointmentId() == appointmentId) return true;
        }
    }
    return false;
}

// Line shapes:
//   P,General,id,name,age,contact,complaint
//   P,Emergency,id,name,age,contact,severity,condition
//   P,FollowUp,id,name,age,contact,previousDiagnosis,lastVisitDate
void parsePatient(const std::vector<std::string>& f, ClinicDatabase& staging, int& maxPatientId) {
    if (f.size() < 7) {
        throw FileException("Patient record has too few fields.");
    }

    int id = 0;
    int age = 0;
    if (!tryParseInt(f[2], id))  throw FileException("Patient ID is not a number: " + f[2]);
    if (!tryParseInt(f[4], age)) throw FileException("Patient age is not a number: " + f[4]);

    std::string name = unescapeField(f[3]);
    std::string contact = unescapeField(f[5]);
    const std::string& type = f[1];

    PatientRecord* patient = nullptr;

    if (type == "General") {
        if (f.size() != 7) throw FileException("Patient record has the wrong number of fields.");
        patient = new GeneralPatient(id, name, age, contact, unescapeField(f[6]));

    } else if (type == "Emergency") {
        if (f.size() != 8) throw FileException("Patient record has the wrong number of fields.");
        int severity = 0;
        if (!tryParseInt(f[6], severity)) throw FileException("Severity is not a number: " + f[6]);
        if (severity < 1 || severity > 5) throw InvalidDataException("Severity must be between 1 and 5.");
        patient = new EmergencyPatient(id, name, age, contact, severity, unescapeField(f[7]));

    } else if (type == "FollowUp") {
        if (f.size() != 8) throw FileException("Patient record has the wrong number of fields.");
        std::string lastVisit = unescapeField(f[7]);
        Validator::validateDate(lastVisit);
        patient = new FollowUpPatient(id, name, age, contact, unescapeField(f[6]), lastVisit);

    } else {
        throw FileException("Unknown patient type: " + type);
    }

    try {
        staging.addPatient(patient);     // validates and checks for duplicate IDs
    } catch (...) {
        delete patient;                  // the database refused it, so we clean up
        throw;
    }

    if (id > maxPatientId) maxPatientId = id;
}

// Line shape: A,appointmentId,patientId,date,time,reason,status
void parseAppointment(const std::vector<std::string>& f, ClinicDatabase& staging, int& maxAppointmentId) {
    if (f.size() != 7) {
        throw FileException("Appointment record must have 7 fields.");
    }

    int appointmentId = 0;
    int patientId = 0;
    if (!tryParseInt(f[1], appointmentId)) throw FileException("Appointment ID is not a number: " + f[1]);
    if (!tryParseInt(f[2], patientId))     throw FileException("Appointment patient ID is not a number: " + f[2]);

    std::string date = unescapeField(f[3]);
    std::string timeText = unescapeField(f[4]);
    std::string reason = unescapeField(f[5]);
    const std::string& statusText = f[6];

    Validator::validateDate(date);

    PatientRecord* patient = staging.getPatientById(patientId);
    if (patient == nullptr) {
        throw FileException("Appointment " + std::to_string(appointmentId) +
                            " refers to unknown patient ID " + std::to_string(patientId) + ".");
    }

    Appointable* schedulable = dynamic_cast<Appointable*>(patient);
    if (schedulable == nullptr) {
        throw FileException("Patient " + std::to_string(patientId) + " (" +
                            patient->getCategory() + ") cannot have appointments.");
    }

    if (appointmentIdInUse(staging, appointmentId)) {
        throw FileException("Appointment ID already exists: " + std::to_string(appointmentId));
    }

    Appointment appointment(appointmentId, patientId, date, timeText, reason);
    if (statusText == "Completed")      appointment.markCompleted();
    else if (statusText == "Cancelled") appointment.cancel();
    else if (statusText != "Pending")   throw FileException("Unknown appointment status: " + statusText);

    schedulable->scheduleAppointment(appointment);
    if (appointmentId > maxAppointmentId) maxAppointmentId = appointmentId;
}

} // anonymous namespace

void FileHandler::load(ClinicDatabase& db, const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw FileException("Cannot open file for reading: " + path);
    }

    // Everything is read into a temporary database first.
    // The real database is touched only if the WHOLE file is valid.
    ClinicDatabase staging;
    int maxPatientId = 0;
    int maxAppointmentId = 0;
    int lineNumber = 0;
    bool headerSeen = false;
    std::string line;

    try {
        while (std::getline(in, line)) {
            lineNumber++;
            if (!line.empty() && line[line.size() - 1] == '\r') {
                line.erase(line.size() - 1);
            }
            if (line.empty()) continue;

            std::vector<std::string> f = splitFields(line, ',');

            if (!headerSeen) {
                if (f.size() != 2 || f[0] != "SCAPMS") {
                    throw FileException("Not a SCAPMS data file (missing header).");
                }
                headerSeen = true;
                continue;
            }

            if (f[0] == "P")      parsePatient(f, staging, maxPatientId);
            else if (f[0] == "A") parseAppointment(f, staging, maxAppointmentId);
            else                  throw FileException("Unknown record type: " + f[0]);
        }
    } catch (const ClinicException& e) {
        // Add the line number so the user knows where the problem is
        throw FileException("Line " + std::to_string(lineNumber) + ": " + e.what());
    }

    if (!headerSeen) {
        throw FileException("The file is empty.");
    }

    // The whole file was valid: replace the real database's contents.
    db.clear();
    while (staging.size() > 0) {
        PatientRecord* p = staging.releasePatient(staging.getAll()[0]->getId());
        db.addPatient(p);
    }

    // New IDs must never clash with the ones we just loaded
    IDGenerator::reservePatientId(maxPatientId);
    IDGenerator::reserveAppointmentId(maxAppointmentId);
}

} // namespace utils
} // namespace scapms