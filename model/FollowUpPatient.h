#ifndef FOLLOWUPPATIENT_H
#define FOLLOWUPPATIENT_H

#include <string>
#include <vector>
#include "model/PatientRecord.h"
#include "model/Appointable.h"
#include "model/AppointmentBook.h"

namespace scapms {
namespace model {

class FollowUpPatient : public PatientRecord, public Appointable {
private:
    std::string previousDiagnosis;
    std::string lastVisitDate;
    AppointmentBook appointments;

public:
    FollowUpPatient(int id, const std::string& name, int age,
                    const std::string& contact,
                    const std::string& previousDiagnosis,
                    const std::string& lastVisitDate);

    const std::string& getPreviousDiagnosis() const;
    const std::string& getLastVisitDate() const;
    void setPreviousDiagnosis(const std::string& diagnosis);
    void setLastVisitDate(const std::string& date);

    std::string getCategory() const override;
    int getPriority() const override;
    std::string getSummary() const override;

    void scheduleAppointment(const Appointment& appointment) override;
    bool cancelAppointment(int appointmentId) override;
    bool completeAppointment(int appointmentId) override;
    bool reopenAppointment(int appointmentId) override;
    int countPendingAppointments() const override;
    const std::vector<Appointment>& getAppointments() const override;
};

} // namespace model
} // namespace scapms

#endif