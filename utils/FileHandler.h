#pragma once
#include "controller/ClinicManager.h"
#include <string>

namespace scapms {

class FileHandler {
public:
    static void save(const ClinicManager& clinic, const std::string& path);
    static void load(ClinicManager& clinic, const std::string& path);   // all-or-nothing
};

} // namespace scapms