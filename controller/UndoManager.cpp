#include "controller/UndoManager.h"

#include <utility>

#include "model/Appointable.h"
#include "model/PatientFactory.h"
#include "utils/ClinicException.h"

namespace scapms {
namespace controller {

using model::Appointable;
using model::PatientRecord;
using utils::InvalidDataException;

// ---------- DeletePatientAction ----------

DeletePatientAction::DeletePatientAction(model::ClinicDatabase& db, PatientRecord* deleted)
    : db(db), deleted(deleted), patientId(0) {
    if (deleted == nullptr) {
        throw InvalidDataException("Cannot record the deletion of an empty patient.");
    }
    patientId = deleted->getId();
    patientName = deleted->getName();
}

DeletePatientAction::~DeletePatientAction() {
    delete deleted;   // still ours only if the delete was never undone
}

std::string DeletePatientAction::describe() const {
    return "Delete patient ID " + std::to_string(patientId) + " (" + patientName + ")";
}

void DeletePatientAction::undo() {
    db.addPatient(deleted);   // may throw; if it does, we still own the patient
    deleted = nullptr;        // the database owns it again
}

// ---------- ModifyPatientAction ----------

ModifyPatientAction::ModifyPatientAction(model::ClinicDatabase& db, int patientId,
                                         const model::PatientData& oldData)
    : db(db), patientId(patientId), oldData(oldData) {}

std::string ModifyPatientAction::describe() const {
    return "Modify patient ID " + std::to_string(patientId) +
           " (restoring " + oldData.name + ", age " + std::to_string(oldData.age) + ")";
}

void ModifyPatientAction::undo() {
    PatientRecord* patient = db.getPatientById(patientId);
    if (patient == nullptr) {
        throw InvalidDataException("Cannot undo: patient " + std::to_string(patientId) +
                                   " no longer exists.");
    }
    model::PatientFactory::apply(*patient, oldData);
}

// ---------- CancelAppointmentAction ----------

CancelAppointmentAction::CancelAppointmentAction(model::ClinicDatabase& db, int patientId,
                                                 int appointmentId)
    : db(db), patientId(patientId), appointmentId(appointmentId) {}

std::string CancelAppointmentAction::describe() const {
    return "Cancel appointment #" + std::to_string(appointmentId) +
           " of patient ID " + std::to_string(patientId);
}

void CancelAppointmentAction::undo() {
    PatientRecord* patient = db.getPatientById(patientId);
    if (patient == nullptr) {
        throw InvalidDataException("Cannot undo: patient " + std::to_string(patientId) +
                                   " no longer exists.");
    }
    Appointable* schedulable = dynamic_cast<Appointable*>(patient);
    if (schedulable == nullptr || !schedulable->reopenAppointment(appointmentId)) {
        throw InvalidDataException("Cannot undo: appointment #" +
                                   std::to_string(appointmentId) + " was not found.");
    }
}

// ---------- UndoManager ----------

void UndoManager::record(std::unique_ptr<UndoAction> action) {
    history.push(std::move(action));
}

std::string UndoManager::undoLast() {
    if (history.empty()) {
        throw InvalidDataException("Nothing to undo.");
    }
    std::string text = history.top()->describe();   // read it BEFORE undoing
    history.top()->undo();                          // if this throws, the action stays on the stack
    history.pop();
    return text;
}

bool UndoManager::canUndo() const { return !history.empty(); }
size_t UndoManager::size() const { return history.size(); }

void UndoManager::clear() {
    while (!history.empty()) history.pop();
}

} // namespace controller
} // namespace scapms