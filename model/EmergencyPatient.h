#ifndef EMERGENCYPATIENT_H
#define EMERGENCYPATIENT_H

#include <string>
#include "model/PatientRecord.h"

namespace scapms {
namespace model {

class EmergencyPatient : public PatientRecord {
private:
    int severityLevel;        // 1 (mild) to 5 (critical)
    std::string condition;    // what brought them in

public:
    EmergencyPatient(int id, const std::string& name, int age,
                     const std::string& contact,
                     int severityLevel, const std::string& condition);

    int getSeverityLevel() const;
    const std::string& getCondition() const;
    void setSeverityLevel(int level);
    void setCondition(const std::string& newCondition);

    std::string getCategory() const override;
    int getPriority() const override;
    std::string getSummary() const override;
};

} // namespace model
} // namespace scapms

#endif