#ifndef RECENTCACHE_H
#define RECENTCACHE_H

#include "model/PatientRecord.h"

namespace scapms {
namespace controller {

// Remembers the 10 most recently used patients.
// Position 0 = most recent. It does NOT own the patients (the database does).
class RecentCache {
public:
    static constexpr int CAPACITY = 10;

private:
    model::PatientRecord* entries[CAPACITY];   // the fixed-size array
    int count;
    int hits;
    int misses;

    int indexOf(int id) const;               // -1 if not cached

public:
    RecentCache();

    model::PatientRecord* lookup(int id);    // nullptr on a miss; a hit moves it to position 0
    void add(model::PatientRecord* patient); // puts it at position 0; drops the oldest if full
    void remove(int id);                     // MUST be called when a patient is deleted
    void clear();

    int size() const;
    int getHits() const;
    int getMisses() const;
    model::PatientRecord* at(int position) const;
};

} // namespace controller
} // namespace scapms

#endif