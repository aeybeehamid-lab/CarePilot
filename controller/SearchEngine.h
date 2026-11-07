#pragma once
#include "model/ClinicDatabase.h"
#include <string>
#include <vector>

namespace scapms {

class SearchEngine {
public:
    // All index-returning functions give -1 when nothing is found.
    static int linearSearchById(const PatientList& list, int id);
    static int binarySearchById(const PatientList& list, int id);   // list MUST be sorted by ID

    // Picks the technique from the data's state: binary if sorted by ID, else linear.
    static int findById(const ClinicDatabase& db, int id);

    // Recursive search: collects the index of every patient whose name contains the keyword.
    static void recursiveSearchByName(const PatientList& list, const std::string& keyword,
                                      size_t index, std::vector<int>& results);
    static std::vector<int> searchByName(const ClinicDatabase& db, const std::string& keyword);

    static std::vector<int> searchByCategory(const PatientList& list, const std::string& category);
};

} // namespace scapms