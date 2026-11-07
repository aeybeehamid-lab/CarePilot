#pragma once
#include "model/ClinicDatabase.h"
#include <string>

namespace carepilot {

class ReportGenerator {
public:
    static std::string mostFrequentlyAttended(const ClinicDatabase& db, size_t topN = 5);
    static std::string pendingAppointments(const ClinicDatabase& db);
    static std::string categoryDistribution(const ClinicDatabase& db);
    static std::string dailySummary(const ClinicDatabase& db, const std::string& date);  // YYYY-MM-DD
    static std::string todayDate();
};

} // namespace carepilot