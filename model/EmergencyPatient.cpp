#include "model/EmergencyPatient.h"

namespace scapms {
namespace model {

EmergencyPatient::EmergencyPatient(int id, const std::string& name, int age,
                                   const std::string& contact,
                                   int severityLevel, const std::string& condition)
    : PatientRecord(id, name, age, contact),
      severityLevel(severityLevel), condition(condition) {}

int EmergencyPatient::getSeverityLevel() const { return severityLevel; }
const std::string& EmergencyPatient::getCondition() const { return condition; }
void EmergencyPatient::setSeverityLevel(int level) { severityLevel = level; }
void EmergencyPatient::setCondition(const std::string& newCondition) { condition = newCondition; }

std::string EmergencyPatient::getCategory() const { return "Emergency"; }

// Most urgent of all three types
int EmergencyPatient::getPriority() const { return 1; }

std::string EmergencyPatient::getSummary() const {
    return "[EMERGENCY] ID: " + std::to_string(getId()) +
           " | Name: " + getName() +
           " | Age: " + std::to_string(getAge()) +
           " | Contact: " + getContact() +
           " | Severity: " + std::to_string(severityLevel) + "/5" +
           " | Condition: " + condition;
}

} // namespace model
} // namespace scapms