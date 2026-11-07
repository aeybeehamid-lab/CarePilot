#pragma once
#include "controller/ClinicManager.h"
#include <string>

namespace carepilot {

class FileHandler {
public:
    static void save(const ClinicManager& clinic, const std::string& path);
    static void load(ClinicManager& clinic, const std::string& path);   // all-or-nothing
};

} // namespace carepilot