#ifndef PATIENTRECORD_H
#define PATIENTRECORD_H

#include <string>

namespace scapms {
namespace model {

class PatientRecord {
private:
    int id;
    std::string name;
    int age;
    std::string contact;

public:
    PatientRecord(int id, const std::string& name, int age, const std::string& contact);
    virtual ~PatientRecord();

    // Getters
    int getId() const;
    const std::string& getName() const;
    int getAge() const;
    const std::string& getContact() const;

    // Setters (id never changes after creation)
    void setName(const std::string& newName);
    void setAge(int newAge);
    void setContact(const std::string& newContact);

    // Pure virtual: every patient type MUST provide these
    virtual std::string getCategory() const = 0;
    virtual int getPriority() const = 0;
    virtual std::string getSummary() const = 0;
};

} // namespace model
} // namespace scapms

#endif