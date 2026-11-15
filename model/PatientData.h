#ifndef PATIENTDATA_H
#define PATIENTDATA_H

#include <string>

namespace scapms {
namespace model {

enum class PatientType { General, Emergency, FollowUp };

// Everything a patient form can hold, in one plain bundle.
// Only the fields for the chosen type are used.
struct PatientData {
    PatientType type = PatientType::General;

    // shared by every type
    std::string name;
    int age = 0;
    std::string contact;

    // General
    std::string complaint;

    // Emergency
    int severityLevel = 1;
    std::string condition;

    // Follow-Up
    std::string previousDiagnosis;
    std::string lastVisitDate;
};

} // namespace model
} // namespace scapms

#endif