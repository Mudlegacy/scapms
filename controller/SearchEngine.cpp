#include "controller/SearchEngine.h"
#include "utils/StringUtils.h"
#include "utils/ClinicException.h"

namespace scapms {
namespace controller {

using model::PatientRecord;

int SearchEngine::lastComparisons = 0;

int SearchEngine::linearSearchById(const std::vector<PatientRecord*>& patients, int id) {
    lastComparisons = 0;
    for (size_t i = 0; i < patients.size(); i++) {
        lastComparisons++;
        if (patients[i]->getId() == id) return static_cast<int>(i);
    }
    return -1;
}

int SearchEngine::binarySearchById(const std::vector<PatientRecord*>& patients, int id) {
    lastComparisons = 0;
    int low = 0;
    int high = static_cast<int>(patients.size()) - 1;

    while (low <= high) {
        int mid = low + (high - low) / 2;
        lastComparisons++;
        int midId = patients[mid]->getId();

        if (midId == id)      return mid;
        else if (midId < id)  low = mid + 1;    // target is in the right half
        else                  high = mid - 1;   // target is in the left half
    }
    return -1;
}

int SearchEngine::binarySearchByName(const std::vector<PatientRecord*>& patients,
                                     const std::string& name) {
    if (name.empty()) {
        throw utils::InvalidDataException("Search keyword cannot be empty.");
    }
    lastComparisons = 0;
    int low = 0;
    int high = static_cast<int>(patients.size()) - 1;

    while (low <= high) {
        int mid = low + (high - low) / 2;
        lastComparisons++;
        int cmp = utils::compareIgnoreCase(patients[mid]->getName(), name);

        if (cmp == 0)     return mid;
        else if (cmp < 0) low = mid + 1;
        else              high = mid - 1;
    }
    return -1;
}

namespace {

// The function that calls itself
int recursiveFind(const std::vector<PatientRecord*>& patients,
                  const std::string& keyword, int index) {
    if (index >= static_cast<int>(patients.size())) return -1;   // base case 1: ran out of patients

    SearchEngine::lastComparisons++;
    if (utils::containsIgnoreCase(patients[index]->getName(), keyword)) {
        return index;                                            // base case 2: found it
    }
    return recursiveFind(patients, keyword, index + 1);          // recursive case: try the next one
}

} // anonymous namespace

int SearchEngine::recursiveSearchByName(const std::vector<PatientRecord*>& patients,
                                        const std::string& keyword, int startIndex) {
    if (keyword.empty()) {
        throw utils::InvalidDataException("Search keyword cannot be empty.");
    }
    if (startIndex < 0) startIndex = 0;
    lastComparisons = 0;
    return recursiveFind(patients, keyword, startIndex);
}

int SearchEngine::findById(const std::vector<PatientRecord*>& patients, int id, bool sortedById) {
    if (sortedById) return binarySearchById(patients, id);
    return linearSearchById(patients, id);
}

} // namespace controller
} // namespace scapms