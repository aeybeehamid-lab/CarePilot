#pragma once
#include "controller/UndoOperation.h"
#include "model/ClinicDatabase.h"
#include "utils/IDGenerator.h"
#include <ctime>
#include <functional>
#include <memory>
#include <queue>
#include <stack>
#include <string>
#include <vector>

namespace scapms {

struct QueueEntry {
    int patientId;
    std::time_t joinedAt;
};

class ClinicManager {
public:
    ClinicManager();

    ClinicDatabase& database();
    const ClinicDatabase& database() const;

    // --- Patients ---
    int suggestPatientId() const;
    void registerPatient(std::unique_ptr<PatientRecord> patient);
    PatientRecord* findPatient(int id);       // null if not found; updates the cache
    void editPatient(int id, const std::function<void(PatientRecord&)>& change);
    void deletePatient(int id);

    // --- Appointments ---
    int  scheduleAppointment(int patientId, const std::string& date,
                             const std::string& time, const std::string& reason);
    void cancelAppointment(int patientId, int appointmentId);
    void completeAppointment(int patientId, int appointmentId);
    void importAppointment(const Appointment& appt);   // used when loading from a file

    // --- Waiting queue (std::queue, first in first out) ---
    void addToQueue(int patientId);
    int  callNextPatient();                   // returns the patient ID called
    std::vector<QueueEntry> waitingList() const;
    size_t queueSize() const;
    long longestWaitSeconds() const;

    // --- Undo last administrative operation (std::stack) ---
    bool canUndo() const;
    std::string undoDescription() const;
    std::string undoLastOperation();

private:
    size_t requireIndex(int id) const;
    void pushUndo(UndoType type, std::unique_ptr<PatientRecord> snapshot,
                  const std::string& description);
    void removeFromQueue(int patientId);

    ClinicDatabase db_;
    IDGenerator patientIds_;
    IDGenerator appointmentIds_;
    std::queue<QueueEntry> queue_;
    std::stack<UndoOperation> undo_;
};

} // namespace scapms