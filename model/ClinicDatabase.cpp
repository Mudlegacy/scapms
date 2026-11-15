#include "model/ClinicDatabase.h"
#include "utils/Validator.h"
#include "utils/ClinicException.h"

namespace scapms {
namespace model {

using namespace scapms::utils;

ClinicDatabase::~ClinicDatabase() {
    clear();
}

void ClinicDatabase::clear() {
    for (size_t i = 0; i < patients.size(); i++) {
        delete patients[i];
    }
    patients.clear();
}

int ClinicDatabase::indexOfId(int id) const {
    for (size_t i = 0; i < patients.size(); i++) {
        if (patients[i]->getId() == id) return static_cast<int>(i);
    }
    return -1;
}

void ClinicDatabase::addPatient(PatientRecord* patient) {
    // Check everything BEFORE changing the database
    if (patient == nullptr) {
        throw InvalidDataException("Cannot add an empty patient record.");
    }
    if (patient->getId() <= 0) {
        throw InvalidDataException("Patient ID must be a positive number.");
    }
    Validator::validateName(patient->getName());
    Validator::validateAge(patient->getAge());
    Validator::validateContact(patient->getContact());

    if (indexOfId(patient->getId()) != -1) {
        throw DuplicateIdException(patient->getId());
    }

    patients.push_back(patient);   // only now does the database take ownership
}

PatientRecord* ClinicDatabase::getPatientById(int id) const {
    int index = indexOfId(id);
    if (index == -1) return nullptr;
    return patients[index];
}

bool ClinicDatabase::idExists(int id) const {
    return indexOfId(id) != -1;
}

PatientRecord* ClinicDatabase::releasePatient(int id) {
    int index = indexOfId(id);
    if (index == -1) {
        throw InvalidDataException("No patient found with ID " + std::to_string(id));
    }
    PatientRecord* patient = patients[index];
    patients.erase(patients.begin() + index);
    return patient;
}

size_t ClinicDatabase::size() const { return patients.size(); }

const std::vector<PatientRecord*>& ClinicDatabase::getAll() const {
    return patients;
}

} // namespace model
} // namespace scapms