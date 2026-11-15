#include "controller/Sorter.h"
#include "utils/StringUtils.h"

namespace scapms {
namespace controller {

using model::PatientRecord;

int Sorter::lastComparisons = 0;

namespace {

// True if a should come BEFORE b for the chosen field.
bool lessThan(const PatientRecord* a, const PatientRecord* b, SortField field) {
    Sorter::lastComparisons++;
    switch (field) {
        case SortField::Name: return utils::compareIgnoreCase(a->getName(), b->getName()) < 0;
        case SortField::Id:   return a->getId() < b->getId();
        case SortField::Age:  return a->getAge() < b->getAge();
    }
    return false;
}

// Merge two sorted halves: [left..mid] and [mid+1..right]
void mergeParts(std::vector<PatientRecord*>& p, std::vector<PatientRecord*>& temp,
                int left, int mid, int right, SortField field) {
    int i = left;
    int j = mid + 1;
    int k = left;

    while (i <= mid && j <= right) {
        // Take from the right half only if it is strictly smaller.
        // On a tie the left item goes first, which keeps the sort stable.
        if (lessThan(p[j], p[i], field)) temp[k++] = p[j++];
        else                             temp[k++] = p[i++];
    }
    while (i <= mid)   temp[k++] = p[i++];
    while (j <= right) temp[k++] = p[j++];

    for (int x = left; x <= right; x++) p[x] = temp[x];
}

// Recursive part of merge sort
void mergeSortRecursive(std::vector<PatientRecord*>& p, std::vector<PatientRecord*>& temp,
                        int left, int right, SortField field) {
    if (left >= right) return;                 // base case: 0 or 1 item is already sorted
    int mid = left + (right - left) / 2;
    mergeSortRecursive(p, temp, left, mid, field);
    mergeSortRecursive(p, temp, mid + 1, right, field);
    mergeParts(p, temp, left, mid, right, field);
}

} // anonymous namespace

void Sorter::insertionSort(std::vector<PatientRecord*>& p, SortField field) {
    lastComparisons = 0;
    int n = static_cast<int>(p.size());

    for (int i = 1; i < n; i++) {
        PatientRecord* key = p[i];     // the item we are placing
        int j = i - 1;
        while (j >= 0 && lessThan(key, p[j], field)) {
            p[j + 1] = p[j];           // shift bigger items one step right
            j--;
        }
        p[j + 1] = key;
    }
}

void Sorter::mergeSort(std::vector<PatientRecord*>& p, SortField field) {
    lastComparisons = 0;
    if (p.size() < 2) return;
    std::vector<PatientRecord*> temp(p.size());
    mergeSortRecursive(p, temp, 0, static_cast<int>(p.size()) - 1, field);
}

void Sorter::sort(std::vector<PatientRecord*>& patients, SortAlgorithm algorithm, SortField field) {
    if (algorithm == SortAlgorithm::Insertion) insertionSort(patients, field);
    else                                       mergeSort(patients, field);
}

} // namespace controller
} // namespace scapms