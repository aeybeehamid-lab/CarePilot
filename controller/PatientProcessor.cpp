#include "controller/PatientProcessor.h"
#include "model/Appointable.h"
#include <sstream>

namespace carepilot {

std::string processPatient(const PatientRecord& p)
{
    std::ostringstream out;
    out << "[" << p.category() << "] #" << p.id() << " " << p.name()
        << " (age " << p.age() << ") - " << p.describe()
        << " | priority " << p.priority();

    // Runtime check: does this patient type implement the Appointable interface?
    if (dynamic_cast<const Appointable*>(&p))
        out << " | can book appointments";
    else
        out << " | walk-in only (goes to queue)";

    return out.str();
}

} // namespace carepilot