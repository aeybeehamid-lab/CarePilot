#include "utils/FileHandler.h"
#include "model/EmergencyPatient.h"
#include "model/FollowUpPatient.h"
#include "model/GeneralPatient.h"
#include "utils/ClinicException.h"
#include <cctype>
#include <fstream>
#include <memory>
#include <vector>

namespace scapms {

static const char* FILE_HEADER = "SCAPMS-DATA-V1";

// '|' separates fields, so it (and line breaks) must not appear inside text.
static std::string clean(const std::string& s)
{
    std::string out = s;
    for (char& c : out)
        if (c == '|' || c == '\n' || c == '\r') c = ' ';
    return out;
}

static std::vector<std::string> splitFields(const std::string& line)
{
    std::vector<std::string> fields;
    std::string current;
    for (char c : line) {
        if (c == '|') { fields.push_back(current); current.clear(); }
        else          { current += c; }
    }
    fields.push_back(current);
    return fields;
}

static int parseInt(const std::string& text, const std::string& what)
{
    if (text.empty())
        throw FileException(what + " is missing.");
    for (char c : text)
        if (!std::isdigit(static_cast<unsigned char>(c)))
            throw FileException(what + " must be a whole number.");
    if (text.size() > 9)
        throw FileException(what + " is too large.");
    return std::stoi(text);
}

// ---------------- Save ----------------

void FileHandler::save(const ClinicManager& clinic, const std::string& path)
{
    std::ofstream out(path);
    if (!out)
        throw FileException("Cannot write to file: " + path);

    out << FILE_HEADER << "\n";
    for (const auto& p : clinic.database().patients()) {
        out << "P|";
        if (auto* g = dynamic_cast<const GeneralPatient*>(p.get())) {
            out << "General|" << p->id() << '|' << clean(p->name()) << '|' << p->age()
                << '|' << clean(p->contact()) << '|' << clean(g->reasonForVisit());
        } else if (auto* e = dynamic_cast<const EmergencyPatient*>(p.get())) {
            out << "Emergency|" << p->id() << '|' << clean(p->name()) << '|' << p->age()
                << '|' << clean(p->contact()) << '|' << e->severity()
                << '|' << clean(e->triageNote());
        } else if (auto* f = dynamic_cast<const FollowUpPatient*>(p.get())) {
            out << "FollowUp|" << p->id() << '|' << clean(p->name()) << '|' << p->age()
                << '|' << clean(p->contact()) << '|' << f->previousVisitDate()
                << '|' << clean(f->doctorName());
        }
        out << "\n";

        for (const Appointment& a : p->appointments())
            out << "A|" << a.id() << '|' << a.patientId() << '|' << a.date() << '|' << a.time()
                << '|' << clean(a.reason()) << '|' << Appointment::statusToText(a.status()) << "\n";
    }

    if (!out)
        throw FileException("An error occurred while writing: " + path);
}

// ---------------- Load ----------------

static void loadPatient(ClinicManager& m, const std::vector<std::string>& f)
{
    if (f.size() < 7)
        throw FileException("Incomplete patient record.");

    const std::string& type = f[1];
    int id  = parseInt(f[2], "Patient ID");
    int age = parseInt(f[4], "Age");

    if (type == "General" && f.size() == 7)
        m.registerPatient(std::make_unique<GeneralPatient>(id, f[3], age, f[5], f[6]));
    else if (type == "Emergency" && f.size() == 8)
        m.registerPatient(std::make_unique<EmergencyPatient>(id, f[3], age, f[5],
                                                             parseInt(f[6], "Severity"), f[7]));
    else if (type == "FollowUp" && f.size() == 8)
        m.registerPatient(std::make_unique<FollowUpPatient>(id, f[3], age, f[5], f[6], f[7]));
    else
        throw FileException("Unknown patient type or wrong number of fields.");
}

static void loadAppointment(ClinicManager& m, const std::vector<std::string>& f)
{
    if (f.size() != 7)
        throw FileException("Incomplete appointment record.");
    m.importAppointment(Appointment(parseInt(f[1], "Appointment ID"),
                                    parseInt(f[2], "Patient ID"),
                                    f[3], f[4], f[5],
                                    Appointment::statusFromText(f[6])));
}

static void stripCR(std::string& line)
{
    if (!line.empty() && line.back() == '\r') line.pop_back();
}

void FileHandler::load(ClinicManager& clinic, const std::string& path)
{
    std::ifstream in(path);
    if (!in)
        throw FileException("Cannot open file: " + path);

    std::string line;
    if (!std::getline(in, line))
        throw FileException("The file is empty.");
    stripCR(line);
    if (line != FILE_HEADER)
        throw FileException("This is not a valid SCAPMS data file.");

    ClinicManager loaded;                      // build into a temporary first
    int lineNo = 1;
    while (std::getline(in, line)) {
        ++lineNo;
        stripCR(line);
        if (line.empty()) continue;

        try {
            std::vector<std::string> fields = splitFields(line);
            if (fields[0] == "P")      loadPatient(loaded, fields);
            else if (fields[0] == "A") loadAppointment(loaded, fields);
            else throw FileException("Unknown record type '" + fields[0] + "'.");
        } catch (const ClinicException& e) {
            throw FileException("Line " + std::to_string(lineNo) + ": " + e.what());
        }
    }
    clinic = std::move(loaded);                // success: replace the live data
}

} // namespace scapms