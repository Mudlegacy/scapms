#ifndef SORTER_H
#define SORTER_H

#include <vector>
#include "model/PatientRecord.h"

namespace scapms {
namespace controller {

enum class SortField { Name, Id, Age };
enum class SortAlgorithm { Insertion, Merge };

class Sorter {
public:
    // How many comparisons the most recent sort made (for our report and defence)
    static int lastComparisons;

    // The GUI combo boxes will call this one
    static void sort(std::vector<model::PatientRecord*>& patients,
                     SortAlgorithm algorithm, SortField field);

    static void insertionSort(std::vector<model::PatientRecord*>& patients, SortField field);
    static void mergeSort(std::vector<model::PatientRecord*>& patients, SortField field);
};

} // namespace controller
} // namespace scapms

#endif