#include "controller/ClinicManager.h"
#include "controller/SearchEngine.h"
#include "model/Appointable.h"
#include "utils/ClinicException.h"
#include <utility>

namespace scapms {

ClinicManager::ClinicManager() : patientIds_(1), appointmentIds_(1) {}

ClinicDatabase& ClinicManager::database() { return db_; }
const ClinicDatabase& ClinicManager::database() const { return db_; }

size_t ClinicManager::requireIndex(int id) const
{
    int index = SearchEngine::findById(db_, id);
    if (index < 0)
        throw PatientNotFoundException("No patient with ID " + std::to_string(id) + ".");
    return static_cast<size_t>(index);
}

void ClinicManager::pushUndo(UndoType type, std::unique_ptr<PatientRecord> snapshot,
                             const std::string& description)
{
    UndoOperation op;
    op.type = type;
    op.snapshot = std::move(snapshot);
    op.description = description;
    undo_.push(std::move(op));
}

// ---------------- Patients ----------------

int ClinicManager::suggestPatientId() const { return patientIds_.peek(); }

void ClinicManager::registerPatient(std::unique_ptr<PatientRecord> patient)
{
    if (!patient)
        throw InvalidPatientException("Cannot register an empty patient record.");
    int id = patient->id();
    db_.addPatient(std::move(patient));     // throws DuplicateIdException if the ID exists
    patientIds_.reserve(id);
}

PatientRecord* ClinicManager::findPatient(int id)
{
    int index = SearchEngine::findById(db_, id);
    if (index < 0) return nullptr;
    PatientRecord* patient = db_.patients()[static_cast<size_t>(index)].get();
    db_.touch(patient);
    return patient;
}

void ClinicManager::editPatient(int id, const std::function<void(PatientRecord&)>& change)
{
    size_t index = requireIndex(id);
    std::unique_ptr<PatientRecord> before  = db_.patients()[index]->clone();
    std::unique_ptr<PatientRecord> working = before->clone();

    change(*working);                       // may throw; the original is untouched

    std::string desc = "Edit of " + before->name() + " (ID " + std::to_string(id) + ")";
    db_.replaceAt(index, std::move(working));
    pushUndo(UndoType::RestoreSnapshot, std::move(before), desc);
}

void ClinicManager::deletePatient(int id)
{
    size_t index = requireIndex(id);
    std::unique_ptr<PatientRecord> removed = db_.removeAt(index);
    removeFromQueue(id);
    std::string desc = "Deletion of " + removed->name() + " (ID " + std::to_string(id) + ")";
    pushUndo(UndoType::DeletePatient, std::move(removed), desc);
}

// ---------------- Appointments ----------------

int ClinicManager::scheduleAppointment(int patientId, const std::string& date,
                                       const std::string& time, const std::string& reason)
{
    PatientRecord* patient = findPatient(patientId);
    if (!patient)
        throw PatientNotFoundException("No patient with ID " + std::to_string(patientId) + ".");

    Appointable* appointable = dynamic_cast<Appointable*>(patient);
    if (!appointable)
        throw InvalidAppointmentException(patient->category() +
            " patients cannot be given scheduled appointments. Add them to the waiting queue instead.");

    int appointmentId = appointmentIds_.next();
    appointable->scheduleAppointment(Appointment(appointmentId, patientId, date, time, reason));
    return appointmentId;
}

void ClinicManager::cancelAppointment(int patientId, int appointmentId)
{
    size_t index = requireIndex(patientId);
    PatientRecord& patient = *db_.patients()[index];

    Appointable* appointable = dynamic_cast<Appointable*>(&patient);
    if (!appointable)
        throw InvalidAppointmentException("This patient has no scheduled appointments.");

    std::unique_ptr<PatientRecord> before = patient.clone();
    if (!appointable->cancelAppointment(appointmentId))
        throw InvalidAppointmentException("Appointment not found, or it is no longer pending.");

    std::string desc = "Cancellation of appointment #" + std::to_string(appointmentId) +
                       " for " + patient.name();
    pushUndo(UndoType::RestoreSnapshot, std::move(before), desc);
}

void ClinicManager::completeAppointment(int patientId, int appointmentId)
{
    PatientRecord* patient = findPatient(patientId);
    if (!patient)
        throw PatientNotFoundException("No patient with ID " + std::to_string(patientId) + ".");
    if (!patient->completeAppointment(appointmentId))
        throw InvalidAppointmentException("Appointment not found, or it is no longer pending.");
}
void ClinicManager::importAppointment(const Appointment& appt)
{
    PatientRecord* patient = findPatient(appt.patientId());
    if (!patient)
        throw PatientNotFoundException("Appointment refers to unknown patient ID " +
                                       std::to_string(appt.patientId()) + ".");
    Appointable* appointable = dynamic_cast<Appointable*>(patient);
    if (!appointable)
        throw InvalidAppointmentException("This patient type cannot have appointments.");
    appointable->scheduleAppointment(appt);
    appointmentIds_.reserve(appt.id());
}
// ---------------- Waiting queue ----------------

void ClinicManager::addToQueue(int patientId)
{
    requireIndex(patientId);                // the patient must exist

    std::queue<QueueEntry> copy = queue_;   // std::queue cannot be iterated, so inspect a copy
    while (!copy.empty()) {
        if (copy.front().patientId == patientId)
            throw ClinicException("That patient is already in the waiting queue.");
        copy.pop();
    }
    queue_.push({patientId, std::time(nullptr)});
}

int ClinicManager::callNextPatient()
{
    if (queue_.empty())
        throw ClinicException("The waiting queue is empty.");

    QueueEntry next = queue_.front();       // first in, first out
    queue_.pop();

    if (!findPatient(next.patientId))
        throw PatientNotFoundException("The next patient no longer exists.");
    return next.patientId;
}

std::vector<QueueEntry> ClinicManager::waitingList() const
{
    std::vector<QueueEntry> list;
    std::queue<QueueEntry> copy = queue_;
    while (!copy.empty()) {
        list.push_back(copy.front());
        copy.pop();
    }
    return list;
}

size_t ClinicManager::queueSize() const { return queue_.size(); }

long ClinicManager::longestWaitSeconds() const
{
    if (queue_.empty()) return 0;
    return static_cast<long>(std::difftime(std::time(nullptr), queue_.front().joinedAt));
}

void ClinicManager::removeFromQueue(int patientId)
{
    std::queue<QueueEntry> kept;
    while (!queue_.empty()) {
        if (queue_.front().patientId != patientId) kept.push(queue_.front());
        queue_.pop();
    }
    queue_ = std::move(kept);
}

// ---------------- Undo ----------------

bool ClinicManager::canUndo() const { return !undo_.empty(); }

std::string ClinicManager::undoDescription() const
{
    return undo_.empty() ? std::string() : undo_.top().description;
}

std::string ClinicManager::undoLastOperation()
{
    if (undo_.empty())
        throw ClinicException("Nothing to undo.");

    UndoOperation op = std::move(undo_.top());
    undo_.pop();

    switch (op.type) {
        case UndoType::DeletePatient:
            db_.addPatient(std::move(op.snapshot));
            break;
        case UndoType::RestoreSnapshot: {
            size_t index = requireIndex(op.snapshot->id());
            db_.replaceAt(index, std::move(op.snapshot));
            break;
        }
    }
    return "Undone: " + op.description;
}

} // namespace scapms