#include <iostream>
#include <string>
#include <vector>
#include "controller/ClinicManager.h"
#include "model/PatientData.h"
#include "utils/ClinicException.h"

using namespace scapms::model;
using namespace scapms::controller;
using namespace scapms::utils;

template <typename Func>
void expectResult(const std::string& label, Func test) {
    try {
        test();
        std::cout << label << ": accepted\n";
    } catch (const ClinicException& e) {
        std::cout << label << ": rejected - " << e.what() << "\n";
    }
}

PatientData makeGeneral(const std::string& name, int age, const std::string& contact,
                        const std::string& complaint) {
    PatientData d;
    d.type = PatientType::General;
    d.name = name; d.age = age; d.contact = contact; d.complaint = complaint;
    return d;
}

PatientData makeEmergency(const std::string& name, int age, const std::string& contact,
                          int severity, const std::string& condition) {
    PatientData d;
    d.type = PatientType::Emergency;
    d.name = name; d.age = age; d.contact = contact;
    d.severityLevel = severity; d.condition = condition;
    return d;
}

PatientData makeFollowUp(const std::string& name, int age, const std::string& contact,
                         const std::string& diagnosis, const std::string& lastVisit) {
    PatientData d;
    d.type = PatientType::FollowUp;
    d.name = name; d.age = age; d.contact = contact;
    d.previousDiagnosis = diagnosis; d.lastVisitDate = lastVisit;
    return d;
}

void printList(const std::string& title, const std::vector<PatientRecord*>& list) {
    std::cout << title << "\n";
    if (list.empty()) {
        std::cout << "  (none)\n";
        return;
    }
    for (PatientRecord* p : list) {
        std::cout << "  ID " << p->getId() << " | " << p->getName()
                  << " | age " << p->getAge() << " | " << p->getCategory() << "\n";
    }
}

int main() {
    ClinicManager clinic;

    // ---------- 1. Register ----------
    std::cout << "--- Register ---\n";
    int ali     = clinic.registerPatient(makeGeneral("Ali Bello", 30, "08012345678", "Headache"));
    int zainab  = clinic.registerPatient(makeEmergency("Zainab Musa", 45, "08098765432", 4, "Chest pain"));
    int ibrahim = clinic.registerPatient(makeFollowUp("Ibrahim Sani", 60, "08055555555", "Hypertension", "2026-10-01"));
    int maryam  = clinic.registerPatient(makeGeneral("Maryam Bello", 22, "08011112222", "Rash"));
    int bala    = clinic.registerPatient(makeGeneral("Bala Danjuma", 52, "08066667777", "Cold"));
    std::cout << "IDs: " << ali << " " << zainab << " " << ibrahim << " " << maryam << " " << bala << "\n";
    expectResult("Register with age 200", [&] {
        clinic.registerPatient(makeGeneral("Too Old", 200, "08000000000", "Cough"));
    });
    std::cout << "Patients: " << clinic.patientCount() << ", waiting: " << clinic.waitingCount() << "\n";

    // ---------- 2. Appointments ----------
    std::cout << "\n--- Appointments ---\n";
    int a1 = clinic.scheduleAppointment(ali, "2026-11-02", "10:30", "Review");
    int a2 = clinic.scheduleAppointment(ibrahim, "2026-11-02", "09:00", "BP check");
    int a3 = clinic.scheduleAppointment(maryam, "2026-11-03", "11:00", "Rash check");
    std::cout << "Appointment IDs: " << a1 << " " << a2 << " " << a3 << "\n";

    expectResult("Schedule for emergency patient", [&] {
        clinic.scheduleAppointment(zainab, "2026-11-04", "09:00", "Checkup");
    });
    expectResult("Schedule with bad time", [&] {
        clinic.scheduleAppointment(ali, "2026-11-04", "25:99", "Checkup");
    });
    expectResult("Schedule with bad date", [&] {
        clinic.scheduleAppointment(ali, "2026-02-30", "09:00", "Checkup");
    });
    expectResult("Schedule with blank reason", [&] {
        clinic.scheduleAppointment(ali, "2026-11-04", "09:00", "   ");
    });
    expectResult("Schedule for unknown patient", [&] {
        clinic.scheduleAppointment(99, "2026-11-04", "09:00", "Checkup");
    });
    expectResult("Double booking", [&] {
        clinic.scheduleAppointment(ali, "2026-11-02", "10:30", "Again");
    });

    clinic.completeAppointment(ibrahim, a2);
    clinic.cancelAppointment(maryam, a3);
    expectResult("Cancel an already-cancelled appointment", [&] {
        clinic.cancelAppointment(maryam, a3);
    });
    expectResult("Complete a cancelled appointment", [&] {
        clinic.completeAppointment(maryam, a3);
    });
    expectResult("Cancel unknown appointment", [&] {
        clinic.cancelAppointment(ali, 999);
    });

    std::cout << "All appointments:\n";
    for (const Appointment& ap : clinic.getAllAppointments()) {
        std::cout << "  " << ap.toString() << "\n";
    }

    // ---------- 3. Waiting queue ----------
    std::cout << "\n--- Waiting queue ---\n";
    clinic.addToQueue(ali);
    clinic.addToQueue(bala);
    clinic.addToQueue(maryam);
    expectResult("Add Ali again", [&] { clinic.addToQueue(ali); });
    printList("Waiting (call order):", clinic.getWaitingPatients());

    PatientRecord* next = clinic.callNextPatient();
    std::cout << "Called: " << next->getName() << " (" << next->getCategory() << ")\n";
    next = clinic.callNextPatient();
    std::cout << "Called: " << next->getName() << " (" << next->getCategory() << ")\n";
    std::cout << "Still waiting: " << clinic.waitingCount() << "\n";

    // ---------- 4. Sorting and searching ----------
    std::cout << "\n--- Sort and search ---\n";
    std::cout << "Order: " << clinic.getSortStatus() << "\n";

    expectResult("Binary search while unsorted", [&] {
        clinic.searchById(maryam, SearchMethod::Binary);
    });
    PatientRecord* found = clinic.searchById(maryam, SearchMethod::Linear);
    std::cout << "Linear search ID " << maryam << ": " << found->getName()
              << " (" << clinic.getLastSearchMethod() << ", "
              << clinic.getLastSearchComparisons() << " comparisons)\n";

    clinic.sortPatients(SortAlgorithm::Insertion, SortField::Name);
    std::cout << "Order: " << clinic.getSortStatus() << " ("
              << clinic.getLastSortComparisons() << " comparisons)\n";
    printList("Patients:", clinic.getPatients());

    std::vector<PatientRecord*> matches = clinic.searchByName("bello");
    printList("Name search \"bello\" (" + clinic.getLastSearchMethod() + ", " +
              std::to_string(clinic.getLastSearchComparisons()) + " comparisons):", matches);

    matches = clinic.searchByName("maryam bello", SearchMethod::Binary);
    printList("Exact name \"maryam bello\" (" + clinic.getLastSearchMethod() + ", " +
              std::to_string(clinic.getLastSearchComparisons()) + " comparisons):", matches);

    clinic.sortPatients(SortAlgorithm::Merge, SortField::Id);
    std::cout << "Order: " << clinic.getSortStatus() << " ("
              << clinic.getLastSortComparisons() << " comparisons)\n";

    found = clinic.searchById(maryam, SearchMethod::Binary);
    std::cout << "Binary search ID " << maryam << ": " << found->getName()
              << " (" << clinic.getLastSearchMethod() << ", "
              << clinic.getLastSearchComparisons() << " comparisons)\n";

    found = clinic.searchById(maryam);   // automatic: the cache already knows her
    std::cout << "ID " << maryam << " via automatic search: " << found->getName()
              << " (" << clinic.getLastSearchMethod() << ", "
              << clinic.getLastSearchComparisons() << " comparisons)\n";

    found = clinic.searchById(99);       // automatic: not cached, list is sorted by ID
    std::cout << "ID 99 via automatic search: " << (found ? "found" : "not found")
              << " (" << clinic.getLastSearchMethod() << ", "
              << clinic.getLastSearchComparisons() << " comparisons)\n";

    expectResult("Binary name search while sorted by ID", [&] {
        clinic.searchByName("maryam bello", SearchMethod::Binary);
    });

    printList("Category 'general':", clinic.searchByCategory("general"));
    printList("Category 'Follow-Up':", clinic.searchByCategory("Follow-Up"));
    expectResult("Unknown category", [&] { clinic.searchByCategory("surgery"); });

    // ---------- 5. Edit, delete, undo ----------
    std::cout << "\n--- Edit, delete and undo ---\n";
    PatientData edit = makeGeneral("Maryam B. Bello", 23, "08011112222", "Rash and itching");
    clinic.editPatient(maryam, edit);
    expectResult("Edit with blank name", [&] {
        clinic.editPatient(maryam, makeGeneral("  ", 23, "08011112222", "Rash"));
    });
    expectResult("Edit unknown patient", [&] { clinic.editPatient(99, edit); });
    std::cout << "After edit: " << clinic.searchById(maryam)->getSummary() << "\n";

    clinic.deletePatient(bala);
    std::cout << "Patients after delete: " << clinic.patientCount()
              << ", waiting: " << clinic.waitingCount() << "\n";
    found = clinic.searchById(bala);
    std::cout << "Deleted patient found? " << (found ? "yes" : "no")
              << " (" << clinic.getLastSearchMethod() << ", "
              << clinic.getLastSearchComparisons() << " comparisons)\n";

    std::cout << "Undo history: " << clinic.undoCount() << "\n";
    while (clinic.canUndo()) {
        std::cout << "Undo: " << clinic.undoLastOperation() << "\n";
    }
    std::cout << "Patients after undo: " << clinic.patientCount()
              << ", waiting: " << clinic.waitingCount() << "\n";
    printList("Patient list:", clinic.getPatients());
    std::cout << "Maryam's appointment after undo:\n";
    for (const Appointment& ap : clinic.getAppointmentsOf(maryam)) {
        std::cout << "  " << ap.toString() << "\n";
    }
    expectResult("Undo with empty history", [&] { clinic.undoLastOperation(); });

    // ---------- 6. Reports and dashboard ----------
    std::cout << "\n--- Reports ---\n";
    std::cout << clinic.reportMostAttended() << "\n";
    std::cout << clinic.reportPending() << "\n";
    std::cout << clinic.reportCategories() << "\n";
    std::cout << clinic.reportDaily("2026-11-02") << "\n";

    DashboardStats s = clinic.getStats();
    std::cout << "--- Dashboard ---\n";
    std::cout << "Patients: " << s.totalPatients << ", waiting: " << s.waitingCount
              << ", undo depth: " << s.undoDepth << "\n";
    std::cout << "Appointments - pending: " << s.pendingAppointments
              << ", completed: " << s.completedAppointments
              << ", cancelled: " << s.cancelledAppointments << "\n";
    std::cout << "Cache hits: " << s.cacheHits << ", misses: " << s.cacheMisses << "\n";

    // ---------- 7. Save, new, load ----------
    std::cout << "\n--- Save, New, Load ---\n";
    clinic.saveToFile("clinic_manager_test.csv");
    clinic.newDatabase();
    std::cout << "After New: " << clinic.patientCount() << " patients, "
              << clinic.waitingCount() << " waiting, undo depth " << clinic.undoCount() << "\n";
    expectResult("Load missing file", [&] { clinic.loadFromFile("nope.csv"); });
    clinic.loadFromFile("clinic_manager_test.csv");
    std::cout << "After Load: " << clinic.patientCount() << " patients, "
              << clinic.waitingCount() << " waiting, undo depth " << clinic.undoCount() << "\n";
    printList("Loaded:", clinic.getPatients());

    int newId = clinic.registerPatient(makeGeneral("Fatima Yusuf", 28, "08077778888", "Fever"));
    std::cout << "New patient ID after load: " << newId << "\n";
    return 0;
}