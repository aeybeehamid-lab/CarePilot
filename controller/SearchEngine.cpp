#include "controller/SearchEngine.h"
#include "utils/ClinicException.h"
#include "utils/StringUtils.h"

namespace carepilot {

int SearchEngine::linearSearchById(const PatientList& list, int id)
{
    for (size_t i = 0; i < list.size(); ++i)
        if (list[i]->id() == id) return static_cast<int>(i);
    return -1;
}

int SearchEngine::binarySearchById(const PatientList& list, int id)
{
    int lo = 0;
    int hi = static_cast<int>(list.size()) - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        int midId = list[static_cast<size_t>(mid)]->id();
        if (midId == id) return mid;
        if (midId < id) lo = mid + 1;
        else            hi = mid - 1;
    }
    return -1;
}

int SearchEngine::findById(const ClinicDatabase& db, int id)
{
    if (db.sortedField() == SortField::Id)
        return binarySearchById(db.patients(), id);     // sorted -> O(log n)
    return linearSearchById(db.patients(), id);          // unsorted -> O(n)
}

void SearchEngine::recursiveSearchByName(const PatientList& list, const std::string& keyword,
                                         size_t index, std::vector<int>& results)
{
    if (index >= list.size()) return;                    // base case: end of list
    if (containsIgnoreCase(list[index]->name(), keyword))
        results.push_back(static_cast<int>(index));
    recursiveSearchByName(list, keyword, index + 1, results);   // recursive case
}

std::vector<int> SearchEngine::searchByName(const ClinicDatabase& db, const std::string& keyword)
{
    if (isBlankText(keyword))
        throw InvalidSearchException("Search keyword cannot be empty.");
    std::vector<int> results;
    recursiveSearchByName(db.patients(), keyword, 0, results);
    return results;
}

std::vector<int> SearchEngine::searchByCategory(const PatientList& list, const std::string& category)
{
    if (isBlankText(category))
        throw InvalidSearchException("Please choose a category to search for.");
    std::vector<int> results;
    std::string wanted = toLower(category);
    for (size_t i = 0; i < list.size(); ++i)
        if (toLower(list[i]->category()) == wanted)
            results.push_back(static_cast<int>(i));
    return results;
}

} // namespace carepilot