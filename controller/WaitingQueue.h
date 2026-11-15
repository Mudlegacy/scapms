#ifndef WAITINGQUEUE_H
#define WAITINGQUEUE_H

#include <queue>
#include <vector>
#include "model/PatientRecord.h"

namespace scapms {
namespace controller {

class WaitingQueue {
private:
    std::queue<int> emergencyLane;   // patient IDs
    std::queue<int> regularLane;

    // Takes the queue BY VALUE (a copy) so the real one is not disturbed
    static bool laneContains(std::queue<int> lane, int id);

    // Rebuilds the lane without the given ID; true if it was there
    static bool removeFromLane(std::queue<int>& lane, int id);

public:
    void join(const model::PatientRecord& patient);   // throws if already waiting
    int callNext();                                   // throws if nobody is waiting
    bool remove(int id);                              // true if the patient was waiting
    bool isWaiting(int id) const;
    bool empty() const;
    size_t size() const;
    std::vector<int> waitingIds() const;              // in the order they will be called
};

} // namespace controller
} // namespace scapms

#endif