#ifndef IDGENERATOR_H
#define IDGENERATOR_H

namespace scapms {
namespace utils {

// Hands out unique IDs. All members are static, so there is one counter
// shared by the whole program and no object is needed.
class IDGenerator {
private:
    static int nextPatient;
    static int nextAppointment;

public:
    static int nextPatientId();
    static int nextAppointmentId();

    // After loading a file, make sure new IDs never clash with loaded ones
    static void reservePatientId(int usedId);
    static void reserveAppointmentId(int usedId);
};

} // namespace utils
} // namespace scapms

#endif