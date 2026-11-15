#ifndef GENERALPATIENT_H
#define GENERALPATIENT_H

#include <string>
#include <vector>
#include "model/PatientRecord.h"
#include "model/Appointable.h"
#include "model/AppointmentBook.h"

namespace scapms {
namespace model {

class GeneralPatient : public PatientRecord, public Appointable {
private:
    std::string complaint;
    AppointmentBook appointments;   // composition: the patient HAS appointments

public:
    GeneralPatient(int id, const std::string& name, int age,
                   const std::string& contact, const std::string& complaint);

    const std::string& getComplaint() const;
    void setComplaint(const std::string& newComplaint);

    // From PatientRecord
    std::string getCategory() const override;
    int getPriority() const override;
    std::string getSummary() const override;

    // From Appointable
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