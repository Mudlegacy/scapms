#include "model/PatientFactory.h"

#include "model/EmergencyPatient.h"
#include "model/FollowUpPatient.h"
#include "model/GeneralPatient.h"
#include "utils/ClinicException.h"
#include "utils/Validator.h"

namespace scapms {
namespace model {

using utils::InvalidDataException;
using utils::Validator;

void PatientFactory::validate(const PatientData& d) {
    Validator::validateName(d.name);
    Validator::validateAge(d.age);
    Validator::validateContact(d.contact);

    switch (d.type) {
        case PatientType::General:
            Validator::validateRequiredText(d.complaint, "Complaint");
            break;
        case PatientType::Emergency:
            if (d.severityLevel < 1 || d.severityLevel > 5) {
                throw InvalidDataException("Severity must be between 1 and 5.");
            }
            Validator::validateRequiredText(d.condition, "Condition");
            break;
        case PatientType::FollowUp:
            Validator::validateRequiredText(d.previousDiagnosis, "Previous diagnosis");
            Validator::validateDate(d.lastVisitDate);
            break;
    }
}

PatientRecord* PatientFactory::create(int id, const PatientData& d) {
    validate(d);    // nothing is allocated if the data is bad

    switch (d.type) {
        case PatientType::General:
            return new GeneralPatient(id, d.name, d.age, d.contact, d.complaint);
        case PatientType::Emergency:
            return new EmergencyPatient(id, d.name, d.age, d.contact,
                                        d.severityLevel, d.condition);
        case PatientType::FollowUp:
            return new FollowUpPatient(id, d.name, d.age, d.contact,
                                       d.previousDiagnosis, d.lastVisitDate);
    }
    throw InvalidDataException("Unknown patient type.");
}

PatientData PatientFactory::extract(const PatientRecord& patient) {
    PatientData d;
    d.name = patient.getName();
    d.age = patient.getAge();
    d.contact = patient.getContact();

    if (const GeneralPatient* g = dynamic_cast<const GeneralPatient*>(&patient)) {
        d.type = PatientType::General;
        d.complaint = g->getComplaint();
    } else if (const EmergencyPatient* e = dynamic_cast<const EmergencyPatient*>(&patient)) {
        d.type = PatientType::Emergency;
        d.severityLevel = e->getSeverityLevel();
        d.condition = e->getCondition();
    } else if (const FollowUpPatient* f = dynamic_cast<const FollowUpPatient*>(&patient)) {
        d.type = PatientType::FollowUp;
        d.previousDiagnosis = f->getPreviousDiagnosis();
        d.lastVisitDate = f->getLastVisitDate();
    }
    return d;
}

void PatientFactory::apply(PatientRecord& patient, const PatientData& d) {
    validate(d);    // reject bad data BEFORE touching the patient

    if (GeneralPatient* g = dynamic_cast<GeneralPatient*>(&patient)) {
        if (d.type != PatientType::General) {
            throw InvalidDataException("Patient type cannot be changed.");
        }
        g->setComplaint(d.complaint);
    } else if (EmergencyPatient* e = dynamic_cast<EmergencyPatient*>(&patient)) {
        if (d.type != PatientType::Emergency) {
            throw InvalidDataException("Patient type cannot be changed.");
        }
        e->setSeverityLevel(d.severityLevel);
        e->setCondition(d.condition);
    } else if (FollowUpPatient* f = dynamic_cast<FollowUpPatient*>(&patient)) {
        if (d.type != PatientType::FollowUp) {
            throw InvalidDataException("Patient type cannot be changed.");
        }
        f->setPreviousDiagnosis(d.previousDiagnosis);
        f->setLastVisitDate(d.lastVisitDate);
    } else {
        throw InvalidDataException("Unknown patient type.");
    }

    patient.setName(d.name);
    patient.setAge(d.age);
    patient.setContact(d.contact);
}

} // namespace model
} // namespace scapms