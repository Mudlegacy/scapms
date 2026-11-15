#ifndef PATIENTFACTORY_H
#define PATIENTFACTORY_H

#include "model/PatientData.h"
#include "model/PatientRecord.h"

namespace scapms {
namespace model {

class PatientFactory {
public:
    // Checks every field for the chosen type. Throws InvalidDataException.
    static void validate(const PatientData& data);

    // Builds the right subclass. Validates first. The CALLER owns the result.
    static PatientRecord* create(int id, const PatientData& data);

    // Copies a patient's current values into a PatientData
    static PatientData extract(const PatientRecord& patient);

    // Overwrites a patient's values. Validates first, so a bad update
    // changes nothing. The patient's type cannot be changed.
    static void apply(PatientRecord& patient, const PatientData& data);
};

} // namespace model
} // namespace scapms

#endif