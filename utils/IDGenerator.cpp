#include "utils/IDGenerator.h"

namespace scapms {
namespace utils {

int IDGenerator::nextPatient = 1;
int IDGenerator::nextAppointment = 1;

int IDGenerator::nextPatientId() { return nextPatient++; }
int IDGenerator::nextAppointmentId() { return nextAppointment++; }

void IDGenerator::reservePatientId(int usedId) {
    if (usedId >= nextPatient) nextPatient = usedId + 1;
}

void IDGenerator::reserveAppointmentId(int usedId) {
    if (usedId >= nextAppointment) nextAppointment = usedId + 1;
}

} // namespace utils
} // namespace scapms