#include "controller/ClinicManager.h"

#include "controller/ReportGenerator.h"
#include "controller/SearchEngine.h"
#include "model/PatientFactory.h"
#include "utils/ClinicException.h"
#include "utils/FileHandler.h"
#include "utils/IDGenerator.h"
#include "utils/StringUtils.h"
#include "utils/Validator.h"

namespace scapms {
namespace controller {

using model::Appointable;
using model::Appointment;
using model::AppointmentStatus;
using model::PatientData;
using model::PatientFactory;
using model::PatientRecord;
using model::PatientType;
using utils::InvalidDataException;

namespace {

// Manual search through one patient's appointments
const Appointment* findAppointment(const Appointable& patient, int appointmentId) {
    const std::vector<Appointment>& list = patient.getAppointments();
    for (size_t i = 0; i < list.size(); i++) {
        if (list[i].getAppointmentId() == appointmentId) return &list[i];
    }
    return nullptr;
}

ViewOrder orderFor(SortField field) {
    switch (field) {
        case SortField::Id:   return ViewOrder::ById;
        case SortField::Name: return ViewOrder::ByName;
        case SortField::Age:  return ViewOrder::ByAge;
    }
    return ViewOrder::Unsorted;
}

} // anonymous namespace

// ============================ helpers ============================

ClinicManager::ClinicManager()
    : viewOrder(ViewOrder::Unsorted),
      lastSearchMethod("None yet"),
      lastSearchComparisons(0) {}

// After any change to the patients, the table list is rebuilt from the
// database. A rebuilt list is in registration order, so it is marked
// Unsorted: binary search can never run on a stale order.
void ClinicManager::rebuildView() {
    view = db.getAll();
    viewOrder = ViewOrder::Unsorted;
}

PatientRecord& ClinicManager::requirePatient(int id) const {
    PatientRecord* patient = db.getPatientById(id);
    if (patient == nullptr) {
        throw InvalidDataException("No patient found with ID " + std::to_string(id) + ".");
    }
    return *patient;
}

Appointable& ClinicManager::requireAppointable(int patientId) const {
    PatientRecord& patient = requirePatient(patientId);
    Appointable* schedulable = dynamic_cast<Appointable*>(&patient);
    if (schedulable == nullptr) {
        throw InvalidDataException(patient.getCategory() +
            " patients cannot have appointments. Use the waiting queue instead.");
    }
    return *schedulable;
}

// ============================ patients ============================

int ClinicManager::registerPatient(const PatientData& data) {
    PatientFactory::validate(data);    // reject bad input BEFORE an ID is used up

    int id = utils::IDGenerator::nextPatientId();
    while (db.idExists(id)) {
        id = utils::IDGenerator::nextPatientId();
    }

    PatientRecord* patient = PatientFactory::create(id, data);
    try {
        db.addPatient(patient);
    } catch (...) {
        delete patient;                // the database refused it, so we still own it
        throw;
    }

    // Emergency patients are walk-ins and cannot be scheduled,
    // so they go straight into the waiting queue.
    if (data.type == PatientType::Emergency) {
        waitingQueue.join(*patient);
    }

    cache.add(patient);
    rebuildView();
    return id;
}

void ClinicManager::editPatient(int id, const PatientData& data) {
    PatientRecord& patient = requirePatient(id);
    PatientData before = PatientFactory::extract(patient);   // snapshot for Undo

    PatientFactory::apply(patient, data);    // validates first; throws without changing anything
    undoManager.record(std::make_unique<ModifyPatientAction>(db, id, before));

    cache.add(&patient);
    rebuildView();
}

void ClinicManager::deletePatient(int id) {
    requirePatient(id);                      // throws if there is no such patient

    // Everything that could still point at this patient must let go FIRST
    cache.remove(id);
    waitingQueue.remove(id);

    PatientRecord* released = db.releasePatient(id);
    undoManager.record(std::make_unique<DeletePatientAction>(db, released));
    rebuildView();
}

size_t ClinicManager::patientCount() const {
    return db.size();
}

const std::vector<PatientRecord*>& ClinicManager::getPatients() const {
    return view;
}

// ============================ appointments ============================

int ClinicManager::scheduleAppointment(int patientId, const std::string& date,
                                       const std::string& timeText, const std::string& reason) {
    utils::Validator::validateDate(date);
    utils::Validator::validateTime(timeText);
    utils::Validator::validateRequiredText(reason, "Reason");

    Appointable& schedulable = requireAppointable(patientId);

    // The same patient cannot be booked twice for the same slot
    const std::vector<Appointment>& existing = schedulable.getAppointments();
    for (size_t i = 0; i < existing.size(); i++) {
        if (existing[i].isPending() && existing[i].getDate() == date &&
            existing[i].getTime() == timeText) {
            throw InvalidDataException(
                "This patient already has a pending appointment at that date and time.");
        }
    }

    int appointmentId = utils::IDGenerator::nextAppointmentId();
    schedulable.scheduleAppointment(Appointment(appointmentId, patientId, date, timeText, reason));
    cache.add(db.getPatientById(patientId));
    return appointmentId;
}

void ClinicManager::cancelAppointment(int patientId, int appointmentId) {
    Appointable& schedulable = requireAppointable(patientId);
    const Appointment* appointment = findAppointment(schedulable, appointmentId);

    if (appointment == nullptr) {
        throw InvalidDataException("No appointment #" + std::to_string(appointmentId) +
                                   " for patient ID " + std::to_string(patientId) + ".");
    }
    if (!appointment->isPending()) {
        throw InvalidDataException("Only pending appointments can be cancelled.");
    }

    schedulable.cancelAppointment(appointmentId);
    undoManager.record(std::make_unique<CancelAppointmentAction>(db, patientId, appointmentId));
}

void ClinicManager::completeAppointment(int patientId, int appointmentId) {
    Appointable& schedulable = requireAppointable(patientId);
    const Appointment* appointment = findAppointment(schedulable, appointmentId);

    if (appointment == nullptr) {
        throw InvalidDataException("No appointment #" + std::to_string(appointmentId) +
                                   " for patient ID " + std::to_string(patientId) + ".");
    }
    if (!appointment->isPending()) {
        throw InvalidDataException("Only pending appointments can be completed.");
    }

    schedulable.completeAppointment(appointmentId);
}

std::vector<Appointment> ClinicManager::getAppointmentsOf(int patientId) const {
    PatientRecord& patient = requirePatient(patientId);
    const Appointable* schedulable = dynamic_cast<const Appointable*>(&patient);
    if (schedulable == nullptr) {
        return std::vector<Appointment>();   // walk-in patients simply have none
    }
    return schedulable->getAppointments();
}

std::vector<Appointment> ClinicManager::getAllAppointments() const {
    std::vector<Appointment> all;
    const std::vector<PatientRecord*>& patients = db.getAll();
    for (size_t i = 0; i < patients.size(); i++) {
        const Appointable* schedulable = dynamic_cast<const Appointable*>(patients[i]);
        if (schedulable == nullptr) continue;
        const std::vector<Appointment>& list = schedulable->getAppointments();
        for (size_t j = 0; j < list.size(); j++) {
            all.push_back(list[j]);
        }
    }
    return all;
}

// ============================ waiting queue ============================

void ClinicManager::addToQueue(int patientId) {
    PatientRecord& patient = requirePatient(patientId);
    waitingQueue.join(patient);              // throws if already waiting
    cache.add(&patient);
}

PatientRecord* ClinicManager::callNextPatient() {
    int id = waitingQueue.callNext();        // throws if nobody is waiting
    PatientRecord* patient = db.getPatientById(id);
    if (patient == nullptr) {
        throw InvalidDataException("The next patient's record no longer exists.");
    }
    cache.add(patient);
    return patient;
}

std::vector<PatientRecord*> ClinicManager::getWaitingPatients() const {
    std::vector<PatientRecord*> result;
    std::vector<int> ids = waitingQueue.waitingIds();
    for (size_t i = 0; i < ids.size(); i++) {
        PatientRecord* patient = db.getPatientById(ids[i]);
        if (patient != nullptr) result.push_back(patient);
    }
    return result;
}

size_t ClinicManager::waitingCount() const {
    return waitingQueue.size();
}

// ============================ sorting ============================

void ClinicManager::sortPatients(SortAlgorithm algorithm, SortField field) {
    Sorter::sort(view, algorithm, field);
    viewOrder = orderFor(field);             // remember WHAT it is sorted by
}

int ClinicManager::getLastSortComparisons() const {
    return Sorter::lastComparisons;
}

std::string ClinicManager::getSortStatus() const {
    switch (viewOrder) {
        case ViewOrder::Unsorted: return "Unsorted (registration order)";
        case ViewOrder::ById:     return "Sorted by ID";
        case ViewOrder::ByName:   return "Sorted by name";
        case ViewOrder::ByAge:    return "Sorted by age";
    }
    return "Unknown";
}

// ============================ searching ============================

PatientRecord* ClinicManager::searchById(int id, SearchMethod method) {
    int index = -1;

    switch (method) {
        case SearchMethod::Automatic: {
            // 1. Try the small recent-patients cache first
            PatientRecord* cached = cache.lookup(id);
            if (cached != nullptr) {
                lastSearchMethod = "Recent-patients cache";
                lastSearchComparisons = 0;
                return cached;
            }
            // 2. Otherwise choose the technique that fits the state of the data
            bool sortedById = (viewOrder == ViewOrder::ById);
            index = SearchEngine::findById(view, id, sortedById);
            lastSearchMethod = sortedById ? "Binary search" : "Linear search";
            break;
        }
        case SearchMethod::Linear:
            index = SearchEngine::linearSearchById(view, id);
            lastSearchMethod = "Linear search";
            break;
        case SearchMethod::Binary:
            if (viewOrder != ViewOrder::ById) {
                throw InvalidDataException(
                    "Binary search needs the list sorted by ID first. Sort by ID and try again.");
            }
            index = SearchEngine::binarySearchById(view, id);
            lastSearchMethod = "Binary search";
            break;
        case SearchMethod::Recursive:
            throw InvalidDataException("Recursive search works on names, not IDs.");
    }

    lastSearchComparisons = SearchEngine::lastComparisons;
    if (index == -1) return nullptr;

    cache.add(view[index]);
    return view[index];
}

std::vector<PatientRecord*> ClinicManager::searchByName(const std::string& keyword,
                                                        SearchMethod method) {
    utils::Validator::validateRequiredText(keyword, "Search keyword");

    if (method == SearchMethod::Linear) {
        throw InvalidDataException(
            "Linear search is used for IDs. Use Recursive or Binary search for names.");
    }

    std::vector<PatientRecord*> matches;

    if (method == SearchMethod::Binary) {
        // Exact name, and only valid on a list sorted by name
        if (viewOrder != ViewOrder::ByName) {
            throw InvalidDataException(
                "Binary search needs the list sorted by name first. Sort by name and try again.");
        }
        int index = SearchEngine::binarySearchByName(view, keyword);
        lastSearchMethod = "Binary search";
        lastSearchComparisons = SearchEngine::lastComparisons;
        if (index == -1) return matches;

        // Equal names sit next to each other, so collect the neighbours too
        int first = index;
        while (first > 0 &&
               utils::compareIgnoreCase(view[first - 1]->getName(), keyword) == 0) {
            first--;
        }
        int last = index;
        while (last + 1 < static_cast<int>(view.size()) &&
               utils::compareIgnoreCase(view[last + 1]->getName(), keyword) == 0) {
            last++;
        }
        for (int i = first; i <= last; i++) {
            matches.push_back(view[i]);
        }
        return matches;
    }

    // Automatic or Recursive: every name that CONTAINS the keyword.
    // Each call to the recursive search finds the next match after 'start'.
    int total = 0;
    int start = 0;
    while (true) {
        int index = SearchEngine::recursiveSearchByName(view, keyword, start);
        total += SearchEngine::lastComparisons;
        if (index == -1) break;
        matches.push_back(view[index]);
        start = index + 1;
    }
    lastSearchMethod = "Recursive search";
    lastSearchComparisons = total;
    return matches;
}

std::vector<PatientRecord*> ClinicManager::searchByCategory(const std::string& category) {
    std::string wanted = utils::toLower(category);
    if (wanted == "followup" || wanted == "follow up") wanted = "follow-up";

    if (wanted != "general" && wanted != "emergency" && wanted != "follow-up") {
        throw InvalidDataException("Unknown category: " + category +
                                   ". Use General, Emergency or Follow-Up.");
    }

    std::vector<PatientRecord*> matches;
    int comparisons = 0;
    for (size_t i = 0; i < view.size(); i++) {
        comparisons++;
        if (utils::compareIgnoreCase(view[i]->getCategory(), wanted) == 0) {
            matches.push_back(view[i]);
        }
    }
    lastSearchMethod = "Linear scan";
    lastSearchComparisons = comparisons;
    return matches;
}

const std::string& ClinicManager::getLastSearchMethod() const {
    return lastSearchMethod;
}

int ClinicManager::getLastSearchComparisons() const {
    return lastSearchComparisons;
}

// ============================ undo ============================

std::string ClinicManager::undoLastOperation() {
    std::string text = undoManager.undoLast();   // throws if there is nothing to undo
    rebuildView();                               // a deleted patient may have come back
    return text;
}

bool ClinicManager::canUndo() const {
    return undoManager.canUndo();
}

size_t ClinicManager::undoCount() const {
    return undoManager.size();
}

// ============================ reports and dashboard ============================

std::string ClinicManager::reportMostAttended(int topN) const {
    return ReportGenerator::mostFrequentlyAttended(db, topN);
}

std::string ClinicManager::reportPending() const {
    return ReportGenerator::patientsWithPending(db);
}

std::string ClinicManager::reportCategories() const {
    return ReportGenerator::categoryDistribution(db);
}

std::string ClinicManager::reportDaily(const std::string& date) const {
    return ReportGenerator::dailySummary(db, date);
}

DashboardStats ClinicManager::getStats() const {
    DashboardStats stats;
    stats.totalPatients = static_cast<int>(db.size());
    stats.waitingCount = static_cast<int>(waitingQueue.size());
    stats.undoDepth = static_cast<int>(undoManager.size());
    stats.cacheHits = cache.getHits();
    stats.cacheMisses = cache.getMisses();

    const std::vector<PatientRecord*>& patients = db.getAll();
    for (size_t i = 0; i < patients.size(); i++) {
        const Appointable* schedulable = dynamic_cast<const Appointable*>(patients[i]);
        if (schedulable == nullptr) continue;

        const std::vector<Appointment>& list = schedulable->getAppointments();
        for (size_t j = 0; j < list.size(); j++) {
            switch (list[j].getStatus()) {
                case AppointmentStatus::Pending:   stats.pendingAppointments++;   break;
                case AppointmentStatus::Completed: stats.completedAppointments++; break;
                case AppointmentStatus::Cancelled: stats.cancelledAppointments++; break;
            }
        }
    }
    return stats;
}

// ============================ files ============================

void ClinicManager::saveToFile(const std::string& path) const {
    utils::Validator::validateRequiredText(path, "File name");
    utils::FileHandler::save(db, path);
}

void ClinicManager::loadFromFile(const std::string& path) {
    utils::Validator::validateRequiredText(path, "File name");

    // If the file is bad this throws and NOTHING has changed
    utils::FileHandler::load(db, path);

    // The old patients are gone, so anything that pointed at them must be reset
    cache.clear();
    waitingQueue = WaitingQueue();
    undoManager.clear();
    rebuildView();
}

void ClinicManager::newDatabase() {
    cache.clear();
    waitingQueue = WaitingQueue();
    undoManager.clear();
    db.clear();
    rebuildView();
}

} // namespace controller
} // namespace scapms