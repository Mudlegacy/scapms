#ifndef CLINICEXCEPTION_H
#define CLINICEXCEPTION_H

#include <stdexcept>
#include <string>

namespace scapms {
namespace utils {

// Base class for every error our system throws on purpose
class ClinicException : public std::runtime_error {
public:
    explicit ClinicException(const std::string& message)
        : std::runtime_error(message) {}
};

// Bad name, age, contact, date, search input...
class InvalidDataException : public ClinicException {
public:
    explicit InvalidDataException(const std::string& message)
        : ClinicException(message) {}
};

// Adding a patient whose ID is already in use
class DuplicateIdException : public ClinicException {
public:
    explicit DuplicateIdException(int id)
        : ClinicException("Patient ID already exists: " + std::to_string(id)) {}
};

// Save/load problems
class FileException : public ClinicException {
public:
    explicit FileException(const std::string& message)
        : ClinicException(message) {}
};

} // namespace utils
} // namespace scapms

#endif