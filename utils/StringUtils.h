#pragma once
#include <cctype>
#include <string>

namespace carepilot {

inline std::string toLower(std::string s)
{
    for (char& c : s)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

inline bool isBlankText(const std::string& s)
{
    for (char c : s)
        if (!std::isspace(static_cast<unsigned char>(c))) return false;
    return true;
}

// Manual substring check, case-insensitive.
inline bool containsIgnoreCase(const std::string& text, const std::string& key)
{
    std::string t = toLower(text);
    std::string k = toLower(key);
    if (k.empty()) return true;
    if (k.size() > t.size()) return false;
    for (size_t i = 0; i + k.size() <= t.size(); ++i) {
        size_t j = 0;
        while (j < k.size() && t[i + j] == k[j]) ++j;
        if (j == k.size()) return true;
    }
    return false;
}

} // namespace carepilot