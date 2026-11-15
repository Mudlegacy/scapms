#include "controller/WaitingQueue.h"
#include "utils/ClinicException.h"

namespace scapms {
namespace controller {

using model::PatientRecord;
using utils::InvalidDataException;

bool WaitingQueue::laneContains(std::queue<int> lane, int id) {
    while (!lane.empty()) {
        if (lane.front() == id) return true;
        lane.pop();
    }
    return false;
}

bool WaitingQueue::removeFromLane(std::queue<int>& lane, int id) {
    bool removed = false;
    std::queue<int> kept;
    while (!lane.empty()) {
        if (lane.front() == id) removed = true;
        else kept.push(lane.front());     // everyone else keeps their place
        lane.pop();
    }
    lane = kept;
    return removed;
}

void WaitingQueue::join(const PatientRecord& patient) {
    if (isWaiting(patient.getId())) {
        throw InvalidDataException("Patient is already in the waiting queue.");
    }
    if (patient.getPriority() == 1) {          // priority 1 = emergency
        emergencyLane.push(patient.getId());
    } else {
        regularLane.push(patient.getId());
    }
}

int WaitingQueue::callNext() {
    int id;
    if (!emergencyLane.empty()) {
        id = emergencyLane.front();
        emergencyLane.pop();
    } else if (!regularLane.empty()) {
        id = regularLane.front();
        regularLane.pop();
    } else {
        throw InvalidDataException("The waiting queue is empty.");
    }
    return id;
}

bool WaitingQueue::remove(int id) {
    bool fromEmergency = removeFromLane(emergencyLane, id);
    bool fromRegular = removeFromLane(regularLane, id);
    return fromEmergency || fromRegular;
}

bool WaitingQueue::isWaiting(int id) const {
    return laneContains(emergencyLane, id) || laneContains(regularLane, id);
}

bool WaitingQueue::empty() const {
    return emergencyLane.empty() && regularLane.empty();
}

size_t WaitingQueue::size() const {
    return emergencyLane.size() + regularLane.size();
}

std::vector<int> WaitingQueue::waitingIds() const {
    std::vector<int> ids;
    std::queue<int> e = emergencyLane;   // copies
    while (!e.empty()) { ids.push_back(e.front()); e.pop(); }
    std::queue<int> r = regularLane;
    while (!r.empty()) { ids.push_back(r.front()); r.pop(); }
    return ids;
}

} // namespace controller
} // namespace scapms