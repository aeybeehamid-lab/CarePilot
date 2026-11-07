#pragma once
#include <stdexcept>
#include <string>

namespace carepilot {

class ClinicException : public std::runtime_error {
public:
    explicit ClinicException(const std::string& msg) : std::runtime_error(msg) {}
};

class InvalidPatientException     : public ClinicException { public: using ClinicException::ClinicException; };
class DuplicateIdException        : public ClinicException { public: using ClinicException::ClinicException; };
class InvalidAppointmentException : public ClinicException { public: using ClinicException::ClinicException; };
class FileException               : public ClinicException { public: using ClinicException::ClinicException; };
class InvalidSearchException      : public ClinicException { public: using ClinicException::ClinicException; };
class PatientNotFoundException    : public ClinicException { public: using ClinicException::ClinicException; };

} // namespace carepilot