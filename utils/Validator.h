#ifndef VALIDATOR_H
#define VALIDATOR_H

#include <string>

namespace scapms {
namespace utils {

// Each function does nothing if the data is fine, and throws if it is not.
class Validator {
public:
    static void validateName(const std::string& name);
    static void validateAge(int age);
    static void validateContact(const std::string& contact);
    static void validateDate(const std::string& date);        // YYYY-MM-DD
    static void validateTime(const std::string& timeText);    // HH:MM, 24-hour
    static void validateRequiredText(const std::string& text, const std::string& fieldName);
};

} // namespace utils
} // namespace scapms

#endif