#ifndef SEARCHENGINE_H
#define SEARCHENGINE_H

#include <string>
#include <vector>
#include "model/PatientRecord.h"

namespace scapms {
namespace controller {

// Every search returns the INDEX of the match in the vector, or -1 if not found.
class SearchEngine {
public:
    static int lastComparisons;

    // Works on any list, sorted or not
    static int linearSearchById(const std::vector<model::PatientRecord*>& patients, int id);

    // The list MUST already be sorted by ID (ascending)
    static int binarySearchById(const std::vector<model::PatientRecord*>& patients, int id);

    // The list MUST already be sorted by name (ascending)
    static int binarySearchByName(const std::vector<model::PatientRecord*>& patients,
                                  const std::string& name);

    // Recursive: first patient at or after startIndex whose name contains the keyword
    static int recursiveSearchByName(const std::vector<model::PatientRecord*>& patients,
                                     const std::string& keyword, int startIndex = 0);

    // Chooses the right technique depending on the state of the data
    static int findById(const std::vector<model::PatientRecord*>& patients,
                        int id, bool sortedById);
};

} // namespace controller
} // namespace scapms

#endif