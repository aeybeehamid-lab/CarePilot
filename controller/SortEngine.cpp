#include "controller/SortEngine.h"
#include "utils/ClinicException.h"
#include "utils/StringUtils.h"
#include <utility>

namespace scapms {

bool SortEngine::lessThan(const PatientRecord& a, const PatientRecord& b, SortField field)
{
    switch (field) {
        case SortField::Id:   return a.id() < b.id();
        case SortField::Age:  return a.age() < b.age();
        case SortField::Name: return toLower(a.name()) < toLower(b.name());
        case SortField::None: break;
    }
    return false;
}

void SortEngine::selectionSort(PatientList& list, SortField field)
{
    for (size_t i = 0; i + 1 < list.size(); ++i) {
        size_t minIndex = i;
        for (size_t j = i + 1; j < list.size(); ++j)
            if (lessThan(*list[j], *list[minIndex], field)) minIndex = j;
        if (minIndex != i) std::swap(list[i], list[minIndex]);
    }
}

void SortEngine::insertionSort(PatientList& list, SortField field)
{
    for (size_t i = 1; i < list.size(); ++i) {
        std::unique_ptr<PatientRecord> key = std::move(list[i]);
        size_t j = i;
        while (j > 0 && lessThan(*key, *list[j - 1], field)) {
            list[j] = std::move(list[j - 1]);
            --j;
        }
        list[j] = std::move(key);
    }
}

// Merge the sorted ranges [lo, mid) and [mid, hi).
static void mergeRanges(PatientList& list, PatientList& tmp,
                        size_t lo, size_t mid, size_t hi, SortField field)
{
    size_t i = lo, j = mid, k = lo;
    while (i < mid && j < hi) {
        if (!SortEngine::lessThan(*list[j], *list[i], field))
            tmp[k++] = std::move(list[i++]);        // left first on ties -> stable
        else
            tmp[k++] = std::move(list[j++]);
    }
    while (i < mid) tmp[k++] = std::move(list[i++]);
    while (j < hi)  tmp[k++] = std::move(list[j++]);
    for (size_t x = lo; x < hi; ++x) list[x] = std::move(tmp[x]);
}

static void mergeSortRange(PatientList& list, PatientList& tmp,
                           size_t lo, size_t hi, SortField field)
{
    if (hi - lo < 2) return;
    size_t mid = lo + (hi - lo) / 2;
    mergeSortRange(list, tmp, lo, mid, field);
    mergeSortRange(list, tmp, mid, hi, field);
    mergeRanges(list, tmp, lo, mid, hi, field);
}

void SortEngine::mergeSort(PatientList& list, SortField field)
{
    PatientList tmp(list.size());
    mergeSortRange(list, tmp, 0, list.size(), field);
}

void SortEngine::sort(ClinicDatabase& db, SortAlgorithm algorithm, SortField field)
{
    if (field == SortField::None)
        throw ClinicException("Please choose a field to sort by.");

    switch (algorithm) {
        case SortAlgorithm::Selection: selectionSort(db.patients(), field); break;
        case SortAlgorithm::Insertion: insertionSort(db.patients(), field); break;
        case SortAlgorithm::Merge:     mergeSort(db.patients(), field);     break;
    }
    db.setSortedField(field);
}

std::string SortEngine::algorithmName(SortAlgorithm algorithm)
{
    switch (algorithm) {
        case SortAlgorithm::Selection: return "Selection Sort";
        case SortAlgorithm::Insertion: return "Insertion Sort";
        case SortAlgorithm::Merge:     return "Merge Sort";
    }
    return "";
}

std::string SortEngine::explain(SortAlgorithm algorithm)
{
    switch (algorithm) {
        case SortAlgorithm::Selection:
            return "Selection Sort: simple and makes very few swaps. Suitable for small lists, "
                   "such as a small clinic's daily patient list.";
        case SortAlgorithm::Insertion:
            return "Insertion Sort: fast on small or nearly sorted lists, e.g. re-sorting after "
                   "one new patient was added to an already sorted list.";
        case SortAlgorithm::Merge:
            return "Merge Sort: always O(n log n) and stable. Best for large record sets and when "
                   "patients with equal values must keep their relative order.";
    }
    return "";
}

} // namespace scapms