#pragma once
#include "model/PatientRecord.h"
#include <memory>
#include <string>

namespace scapms {

enum class UndoType {
    DeletePatient,      // undo: put the deleted patient back
    RestoreSnapshot     // undo: replace the patient with the saved copy (edit / cancel appointment)
};

struct UndoOperation {
    UndoType type = UndoType::RestoreSnapshot;
    std::unique_ptr<PatientRecord> snapshot;   // copy of the patient BEFORE the change
    std::string description;                   // shown to the user
};

} // namespace scapms