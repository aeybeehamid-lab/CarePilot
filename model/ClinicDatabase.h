#pragma once
#include "model/PatientRecord.h"
#include <cstddef>
#include <memory>
#include <vector>

namespace carepilot {

using PatientList = std::vector<std::unique_ptr<PatientRecord>>;

enum class SortField { None, Id, Name, Age };

class ClinicDatabase {
public:
    static constexpr int CACHE_SIZE = 10;

    ClinicDatabase();

    // --- Vector storage ---
    void addPatient(std::unique_ptr<PatientRecord> patient);
    std::unique_ptr<PatientRecord> removeAt(size_t index);
    void replaceAt(size_t index, std::unique_ptr<PatientRecord> patient);
    bool containsId(int id) const;
    size_t size() const;
    void clear();
    PatientList& patients();
    const PatientList& patients() const;

    // --- Sorted-state tracking ---
    SortField sortedField() const;
    void setSortedField(SortField field);

    // --- Fixed-size array cache: recently accessed patients (most recent last) ---
    void touch(PatientRecord* patient);
    int cacheCount() const;
    PatientRecord* cachedAt(int slot) const;

private:
    void removeFromCache(const PatientRecord* patient);

    PatientList patients_;
    SortField sortedField_;
    PatientRecord* recent_[CACHE_SIZE];   // non-owning pointers
    int recentCount_;
};

} // namespace carepilot