#include "model/PatientRecord.h"

namespace scapms {
namespace model {

PatientRecord::PatientRecord(int id, const std::string& name, int age, const std::string& contact)
    : id(id), name(name), age(age), contact(contact) {}

PatientRecord::~PatientRecord() {}

int PatientRecord::getId() const { return id; }
const std::string& PatientRecord::getName() const { return name; }
int PatientRecord::getAge() const { return age; }
const std::string& PatientRecord::getContact() const { return contact; }

void PatientRecord::setName(const std::string& newName) { name = newName; }
void PatientRecord::setAge(int newAge) { age = newAge; }
void PatientRecord::setContact(const std::string& newContact) { contact = newContact; }

} // namespace model
} // namespace scapms