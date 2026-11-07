#include "controller/ReportGenerator.h"
#include "model/Appointment.h"
#include "utils/ClinicException.h"
#include <ctime>
#include <sstream>
#include <vector>

namespace scapms {

std::string ReportGenerator::todayDate()
{
    std::time_t now = std::time(nullptr);
    std::tm* t = std::localtime(&now);
    char buffer[16];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", t);
    return buffer;
}

std::string ReportGenerator::mostFrequentlyAttended(const ClinicDatabase& db, size_t topN)
{
    struct Row { int id; std::string name; int completed; };
    std::vector<Row> rows;
    for (const auto& p : db.patients()) {
        int c = p->countByStatus(AppointmentStatus::Completed);
        if (c > 0) rows.push_back({p->id(), p->name(), c});
    }

    // Insertion sort, highest count first.
    for (size_t i = 1; i < rows.size(); ++i) {
        Row key = rows[i];
        size_t j = i;
        while (j > 0 && rows[j - 1].completed < key.completed) {
            rows[j] = rows[j - 1];
            --j;
        }
        rows[j] = key;
    }

    std::ostringstream out;
    out << "MOST FREQUENTLY ATTENDED PATIENTS\n---------------------------------\n";
    if (rows.empty()) {
        out << "No completed appointments yet.\n";
        return out.str();
    }
    for (size_t i = 0; i < rows.size() && i < topN; ++i)
        out << (i + 1) << ". " << rows[i].name << " (ID " << rows[i].id << ") - "
            << rows[i].completed << " completed\n";
    return out.str();
}

std::string ReportGenerator::pendingAppointments(const ClinicDatabase& db)
{
    std::ostringstream out;
    out << "PATIENTS WITH PENDING APPOINTMENTS\n----------------------------------\n";
    int shown = 0;
    for (const auto& p : db.patients()) {
        int pending = p->countByStatus(AppointmentStatus::Pending);
        if (pending == 0) continue;

        std::string next;                       // earliest pending "date time"
        for (const Appointment& a : p->appointments()) {
            if (a.status() != AppointmentStatus::Pending) continue;
            std::string key = a.date() + " " + a.time();
            if (next.empty() || key < next) next = key;
        }
        out << p->name() << " (ID " << p->id() << ") - " << pending
            << " pending, next: " << next << "\n";
        ++shown;
    }
    if (shown == 0) out << "No pending appointments.\n";
    return out.str();
}

std::string ReportGenerator::categoryDistribution(const ClinicDatabase& db)
{
    int general = 0, emergency = 0, followUp = 0;
    for (const auto& p : db.patients()) {
        std::string c = p->category();
        if (c == "General")        ++general;
        else if (c == "Emergency") ++emergency;
        else if (c == "Follow-Up") ++followUp;
    }
    std::ostringstream out;
    out << "PATIENT CATEGORY DISTRIBUTION\n-----------------------------\n"
        << "General Patients:   " << general   << "\n"
        << "Emergency Patients: " << emergency << "\n"
        << "Follow-Up Patients: " << followUp  << "\n"
        << "Total:              " << db.size() << "\n";
    return out.str();
}

std::string ReportGenerator::dailySummary(const ClinicDatabase& db, const std::string& date)
{
    if (!Appointment::isValidDate(date))
        throw InvalidAppointmentException("Invalid date. Use the format YYYY-MM-DD.");

    int total = 0, completed = 0, pending = 0, cancelled = 0;
    for (const auto& p : db.patients()) {
        for (const Appointment& a : p->appointments()) {
            if (a.date() != date) continue;
            ++total;
            switch (a.status()) {
                case AppointmentStatus::Completed: ++completed; break;
                case AppointmentStatus::Pending:   ++pending;   break;
                case AppointmentStatus::Cancelled: ++cancelled; break;
            }
        }
    }
    std::ostringstream out;
    out << "DAILY APPOINTMENT SUMMARY - " << date << "\n"
        << "----------------------------------------\n"
        << "Total Appointments: " << total     << "\n"
        << "Completed:          " << completed << "\n"
        << "Pending:            " << pending   << "\n"
        << "Cancelled:          " << cancelled << "\n";
    return out.str();
}

} // namespace scapms