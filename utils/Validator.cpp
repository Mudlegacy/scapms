#include "utils/Validator.h"
#include "utils/ClinicException.h"

namespace scapms {
namespace utils {

static bool isDigit(char c) { return c >= '0' && c <= '9'; }

static int daysInMonth(int year, int month) {
    switch (month) {
        case 4: case 6: case 9: case 11:
            return 30;
        case 2: {
            bool leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
            return leap ? 29 : 28;
        }
        default:
            return 31;
    }
}

void Validator::validateName(const std::string& name) {
    bool hasLetter = false;
    for (size_t i = 0; i < name.size(); i++) {
        if (name[i] != ' ') { hasLetter = true; break; }
    }
    if (!hasLetter) {
        throw InvalidDataException("Patient name cannot be empty.");
    }
}

void Validator::validateAge(int age) {
    if (age < 0 || age > 120) {
        throw InvalidDataException("Patient age must be between 0 and 120.");
    }
}

void Validator::validateContact(const std::string& contact) {
    size_t start = 0;
    if (!contact.empty() && contact[0] == '+') start = 1;   // allow +234...

    size_t digits = contact.size() - start;
    if (digits < 7 || digits > 15) {
        throw InvalidDataException("Contact must have 7 to 15 digits.");
    }
    for (size_t i = start; i < contact.size(); i++) {
        if (!isDigit(contact[i])) {
            throw InvalidDataException("Contact can only contain digits (and a leading +).");
        }
    }
}

void Validator::validateDate(const std::string& date) {
    if (date.size() != 10 || date[4] != '-' || date[7] != '-') {
        throw InvalidDataException("Date must be in the format YYYY-MM-DD.");
    }
    for (size_t i = 0; i < date.size(); i++) {
        if (i == 4 || i == 7) continue;
        if (!isDigit(date[i])) {
            throw InvalidDataException("Date must be in the format YYYY-MM-DD.");
        }
    }
    int year  = (date[0] - '0') * 1000 + (date[1] - '0') * 100 +
                (date[2] - '0') * 10 + (date[3] - '0');
    int month = (date[5] - '0') * 10 + (date[6] - '0');
    int day   = (date[8] - '0') * 10 + (date[9] - '0');

    if (month < 1 || month > 12 || day < 1 || day > daysInMonth(year, month)) {
        throw InvalidDataException("Date has an invalid month or day.");
    }
}

void Validator::validateTime(const std::string& timeText) {
    if (timeText.size() != 5 || timeText[2] != ':' ||
        !isDigit(timeText[0]) || !isDigit(timeText[1]) ||
        !isDigit(timeText[3]) || !isDigit(timeText[4])) {
        throw InvalidDataException("Time must be in the format HH:MM.");
    }
    int hour   = (timeText[0] - '0') * 10 + (timeText[1] - '0');
    int minute = (timeText[3] - '0') * 10 + (timeText[4] - '0');
    if (hour > 23 || minute > 59) {
        throw InvalidDataException("Time has an invalid hour or minute.");
    }
}

void Validator::validateRequiredText(const std::string& text, const std::string& fieldName) {
    for (size_t i = 0; i < text.size(); i++) {
        if (text[i] != ' ') return;      // found something that is not a space
    }
    throw InvalidDataException(fieldName + " cannot be empty.");
}

} // namespace utils
} // namespace scapms