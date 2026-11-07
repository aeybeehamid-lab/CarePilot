#include "model/ClinicDatabase.h"
#include "utils/ClinicException.h"

namespace carepilot {

ClinicDatabase::ClinicDatabase()
    : sortedField_(SortField::None), recentCount_(0)
{
    for (int i = 0; i < CACHE_SIZE; ++i) recent_[i] = nullptr;
}

void ClinicDatabase::addPatient(std::unique_ptr<PatientRecord> patient)
{
    if (!patient)
        throw InvalidPatientException("Cannot add an empty patient record.");
    if (containsId(patient->id()))
        throw DuplicateIdException("Patient ID already exists.");
    patients_.push_back(std::move(patient));
    sortedField_ = SortField::None;     // a new record at the end breaks any sorted order
}

std::unique_ptr<PatientRecord> ClinicDatabase::removeAt(size_t index)
{
    if (index >= patients_.size())
        throw PatientNotFoundException("Patient not found.");
    removeFromCache(patients_[index].get());
    std::unique_ptr<PatientRecord> removed = std::move(patients_[index]);
    patients_.erase(patients_.begin() + static_cast<std::ptrdiff_t>(index));
    return removed;                      // caller keeps it (e.g. for Undo)
}
void ClinicDatabase::replaceAt(size_t index, std::unique_ptr<PatientRecord> patient)
{
    if (index >= patients_.size())
        throw PatientNotFoundException("Patient not found.");
    if (!patient)
        throw InvalidPatientException("Cannot store an empty patient record.");
    removeFromCache(patients_[index].get());   // the old object is about to be destroyed
    patients_[index] = std::move(patient);
    sortedField_ = SortField::None;            // key fields may have changed
}

bool ClinicDatabase::containsId(int id) const
{
    for (const auto& p : patients_)
        if (p->id() == id) return true;
    return false;
}

size_t ClinicDatabase::size() const { return patients_.size(); }

void ClinicDatabase::clear()
{
    patients_.clear();
    for (int i = 0; i < CACHE_SIZE; ++i) recent_[i] = nullptr;
    recentCount_ = 0;
    sortedField_ = SortField::None;
}

PatientList& ClinicDatabase::patients() { return patients_; }
const PatientList& ClinicDatabase::patients() const { return patients_; }

SortField ClinicDatabase::sortedField() const { return sortedField_; }
void ClinicDatabase::setSortedField(SortField field) { sortedField_ = field; }

void ClinicDatabase::touch(PatientRecord* patient)
{
    if (!patient) return;

    // Already cached: move it to the "most recent" end.
    for (int i = 0; i < recentCount_; ++i) {
        if (recent_[i] == patient) {
            for (int j = i; j + 1 < recentCount_; ++j) recent_[j] = recent_[j + 1];
            recent_[recentCount_ - 1] = patient;
            return;
        }
    }

    // Cache full: drop the oldest entry (slot 0).
    if (recentCount_ == CACHE_SIZE) {
        for (int j = 0; j + 1 < CACHE_SIZE; ++j) recent_[j] = recent_[j + 1];
        --recentCount_;
    }
    recent_[recentCount_++] = patient;
}

int ClinicDatabase::cacheCount() const { return recentCount_; }

void ClinicDatabase::clearCache()
{
    for (int i = 0; i < CACHE_SIZE; ++i) recent_[i] = nullptr;
    recentCount_ = 0;
}

PatientRecord* ClinicDatabase::cachedAt(int slot) const
{
    if (slot < 0 || slot >= recentCount_) return nullptr;
    return recent_[slot];
}

void ClinicDatabase::removeFromCache(const PatientRecord* patient)
{
    for (int i = 0; i < recentCount_; ++i) {
        if (recent_[i] == patient) {
            for (int j = i; j + 1 < recentCount_; ++j) recent_[j] = recent_[j + 1];
            --recentCount_;
            recent_[recentCount_] = nullptr;
            return;
        }
    }
}

} // namespace carepilot