#include "controller/RecentCache.h"

namespace scapms {
namespace controller {

using model::PatientRecord;

RecentCache::RecentCache() : count(0), hits(0), misses(0) {
    for (int i = 0; i < CAPACITY; i++) entries[i] = nullptr;
}

int RecentCache::indexOf(int id) const {
    for (int i = 0; i < count; i++) {
        if (entries[i]->getId() == id) return i;
    }
    return -1;
}

PatientRecord* RecentCache::lookup(int id) {
    int index = indexOf(id);
    if (index == -1) {
        misses++;
        return nullptr;
    }
    hits++;
    PatientRecord* found = entries[index];
    for (int i = index; i > 0; i--) entries[i] = entries[i - 1];   // shift right
    entries[0] = found;                                            // move to front
    return found;
}

void RecentCache::add(PatientRecord* patient) {
    if (patient == nullptr) return;

    int index = indexOf(patient->getId());
    if (index != -1) {                       // already cached: just move to front
        for (int i = index; i > 0; i--) entries[i] = entries[i - 1];
        entries[0] = patient;
        return;
    }

    // New entry. If the array is full, the oldest (last) position is overwritten.
    int last = (count < CAPACITY) ? count : CAPACITY - 1;
    for (int i = last; i > 0; i--) entries[i] = entries[i - 1];
    entries[0] = patient;
    if (count < CAPACITY) count++;
}

void RecentCache::remove(int id) {
    int index = indexOf(id);
    if (index == -1) return;
    for (int i = index; i < count - 1; i++) entries[i] = entries[i + 1];   // close the gap
    entries[count - 1] = nullptr;
    count--;
}

void RecentCache::clear() {
    for (int i = 0; i < CAPACITY; i++) entries[i] = nullptr;
    count = 0;
}

int RecentCache::size() const { return count; }
int RecentCache::getHits() const { return hits; }
int RecentCache::getMisses() const { return misses; }

PatientRecord* RecentCache::at(int position) const {
    if (position < 0 || position >= count) return nullptr;
    return entries[position];
}

} // namespace controller
} // namespace scapms