#ifndef CLINICDATABASE_H
#define CLINICDATABASE_H

#include <vector>
#include "model/PatientRecord.h"

namespace scapms {
namespace model {

// Owns every registered patient. Once a patient is added, the database
// is responsible for deleting it.
class ClinicDatabase {
private:
    std::vector<PatientRecord*> patients;

    int indexOfId(int id) const;   // -1 if not found

public:
    ClinicDatabase() = default;
    ~ClinicDatabase();

    // Copying would make two databases delete the same patients
    ClinicDatabase(const ClinicDatabase&) = delete;
    ClinicDatabase& operator=(const ClinicDatabase&) = delete;

    void addPatient(PatientRecord* patient);          // validates, may throw
    PatientRecord* getPatientById(int id) const;      // nullptr if not found
    bool idExists(int id) const;

    // Removes the patient from the database WITHOUT deleting it and hands
    // ownership to the caller (needed later for Undo).
    PatientRecord* releasePatient(int id);

    size_t size() const;
    const std::vector<PatientRecord*>& getAll() const;
    void clear();                                     // deletes everything
};

} // namespace model
} // namespace scapms

#endif