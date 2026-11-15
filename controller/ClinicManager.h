#ifndef CLINICMANAGER_H
#define CLINICMANAGER_H

#include <string>
#include <vector>

#include "controller/RecentCache.h"
#include "controller/Sorter.h"
#include "controller/UndoManager.h"
#include "controller/WaitingQueue.h"
#include "model/Appointable.h"
#include "model/Appointment.h"
#include "model/ClinicDatabase.h"
#include "model/PatientData.h"
#include "model/PatientRecord.h"

namespace scapms {
namespace controller {

// Which search the user chose (the Search tab's combo box will use this)
enum class SearchMethod { Automatic, Linear, Binary, Recursive };

// How the patient list is currently ordered
enum class ViewOrder { Unsorted, ById, ByName, ByAge };

// Numbers for the dashboard panel
struct DashboardStats {
    int totalPatients = 0;
    int waitingCount = 0;
    int pendingAppointments = 0;
    int completedAppointments = 0;
    int cancelledAppointments = 0;
    int undoDepth = 0;
    int cacheHits = 0;
    int cacheMisses = 0;
};

// The one class the GUI talks to. It owns the database, waiting queue,
// recent-patients cache and undo stack, and keeps them consistent.
// Every method that can fail throws a ClinicException.
class ClinicManager {
private:
    model::ClinicDatabase db;       // owns every patient
    WaitingQueue waitingQueue;
    RecentCache cache;
    UndoManager undoManager;

    // The list the patient table shows: a copy of the database's pointers
    // (it does NOT own them). Sorting reorders this list, not the database.
    std::vector<model::PatientRecord*> view;
    ViewOrder viewOrder;

    std::string lastSearchMethod;
    int lastSearchComparisons;

    void rebuildView();
    model::PatientRecord& requirePatient(int id) const;
    model::Appointable& requireAppointable(int patientId) const;

public:
    ClinicManager();

    // ---- Patients ----
    int registerPatient(const model::PatientData& data);          // returns the new ID
    void editPatient(int id, const model::PatientData& data);
    void deletePatient(int id);
    size_t patientCount() const;
    const std::vector<model::PatientRecord*>& getPatients() const;

    // ---- Appointments ----
    int scheduleAppointment(int patientId, const std::string& date,
                            const std::string& timeText, const std::string& reason);
    void cancelAppointment(int patientId, int appointmentId);
    void completeAppointment(int patientId, int appointmentId);
    std::vector<model::Appointment> getAppointmentsOf(int patientId) const;
    std::vector<model::Appointment> getAllAppointments() const;

    // ---- Waiting queue ----
    void addToQueue(int patientId);
    model::PatientRecord* callNextPatient();
    std::vector<model::PatientRecord*> getWaitingPatients() const;   // in call order
    size_t waitingCount() const;

    // ---- Sorting ----
    void sortPatients(SortAlgorithm algorithm, SortField field);
    int getLastSortComparisons() const;
    std::string getSortStatus() const;

    // ---- Searching ----
    model::PatientRecord* searchById(int id, SearchMethod method = SearchMethod::Automatic);
    std::vector<model::PatientRecord*> searchByName(const std::string& keyword,
                                                    SearchMethod method = SearchMethod::Recursive);
    std::vector<model::PatientRecord*> searchByCategory(const std::string& category);
    const std::string& getLastSearchMethod() const;
    int getLastSearchComparisons() const;

    // ---- Undo ----
    std::string undoLastOperation();
    bool canUndo() const;
    size_t undoCount() const;

    // ---- Reports and dashboard ----
    std::string reportMostAttended(int topN = 5) const;
    std::string reportPending() const;
    std::string reportCategories() const;
    std::string reportDaily(const std::string& date) const;
    DashboardStats getStats() const;

    // ---- Files ----
    void saveToFile(const std::string& path) const;
    void loadFromFile(const std::string& path);
    void newDatabase();
};

} // namespace controller
} // namespace scapms

#endif