#pragma once
#include "model/PatientRecord.h"
#include <string>

namespace carepilot {

// Works on ANY patient type through a base-class reference.
std::string processPatient(const PatientRecord& patient);

} // namespace carepilot