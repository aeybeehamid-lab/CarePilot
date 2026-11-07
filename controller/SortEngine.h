#pragma once
#include "model/ClinicDatabase.h"
#include <string>

namespace carepilot {

enum class SortAlgorithm { Selection, Insertion, Merge };

class SortEngine {
public:
    static void sort(ClinicDatabase& db, SortAlgorithm algorithm, SortField field);

    static void selectionSort(PatientList& list, SortField field);
    static void insertionSort(PatientList& list, SortField field);
    static void mergeSort(PatientList& list, SortField field);

    static bool lessThan(const PatientRecord& a, const PatientRecord& b, SortField field);
    static std::string algorithmName(SortAlgorithm algorithm);
    static std::string explain(SortAlgorithm algorithm);   // why each algorithm suits a situation
};

} // namespace carepilot