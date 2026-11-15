#ifndef UNDOMANAGER_H
#define UNDOMANAGER_H

#include <memory>
#include <stack>
#include <string>
#include "model/ClinicDatabase.h"
#include "model/PatientData.h"
#include "model/PatientRecord.h"

namespace scapms {
namespace controller {

// Abstract: every undoable operation knows how to describe and reverse itself
class UndoAction {
public:
    virtual ~UndoAction() = default;
    virtual std::string describe() const = 0;
    virtual void undo() = 0;
};

// A patient was deleted. This action OWNS the deleted patient (with all its
// appointments) until it is undone or destroyed.
class DeletePatientAction : public UndoAction {
private:
    model::ClinicDatabase& db;
    model::PatientRecord* deleted;
    int patientId;
    std::string patientName;

public:
    DeletePatientAction(model::ClinicDatabase& db, model::PatientRecord* deleted);
    ~DeletePatientAction() override;
    std::string describe() const override;
    void undo() override;
};

// A patient was edited. Stores a full snapshot of the OLD values.
class ModifyPatientAction : public UndoAction {
private:
    model::ClinicDatabase& db;
    int patientId;
    model::PatientData oldData;

public:
    ModifyPatientAction(model::ClinicDatabase& db, int patientId,
                        const model::PatientData& oldData);
    std::string describe() const override;
    void undo() override;
};

// An appointment was cancelled. Undo sets it back to Pending.
class CancelAppointmentAction : public UndoAction {
private:
    model::ClinicDatabase& db;
    int patientId;
    int appointmentId;

public:
    CancelAppointmentAction(model::ClinicDatabase& db, int patientId, int appointmentId);
    std::string describe() const override;
    void undo() override;
};

// The stack itself
class UndoManager {
private:
    std::stack<std::unique_ptr<UndoAction>> history;

public:
    void record(std::unique_ptr<UndoAction> action);
    std::string undoLast();          // returns what was undone; throws if nothing to undo
    bool canUndo() const;
    size_t size() const;
    void clear();
};

} // namespace controller
} // namespace scapms

#endif