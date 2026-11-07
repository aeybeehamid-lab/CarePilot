#pragma once
#include "controller/ClinicManager.h"
#include <QDialog>
#include <memory>
#include <string>

class QComboBox;
class QLineEdit;
class QStackedWidget;

namespace scapms::gui {

// Plain data collected from the dialog's fields.
struct PatientForm {
    int id = 0;
    std::string name;
    int age = 0;
    std::string contact;
    int type = 0;                    // 0 = General, 1 = Emergency, 2 = Follow-Up
    std::string reason;              // General
    int severity = 1;                // Emergency
    std::string triageNote;          // Emergency
    std::string previousVisitDate;   // Follow-Up
    std::string doctorName;          // Follow-Up
};

class PatientDialog : public QDialog {
    Q_OBJECT
public:
    // existing == nullptr -> "Add" mode, otherwise "Edit" mode for that patient.
    PatientDialog(const ClinicManager& clinic, const PatientRecord* existing,
                  QWidget* parent = nullptr);

    const PatientForm& form() const;

    static std::unique_ptr<PatientRecord> makePatient(const PatientForm& f);
    static void applyForm(PatientRecord& patient, const PatientForm& f);

protected:
    void accept() override;       // validates before closing

private slots:
    void onTypeChanged(int index);

private:
    PatientForm readForm() const;
    void fillFrom(const PatientRecord& patient);

    const ClinicManager& clinic_;
    bool editing_;
    PatientForm form_;

    QLineEdit* idEdit_;
    QLineEdit* nameEdit_;
    QLineEdit* ageEdit_;
    QLineEdit* contactEdit_;
    QComboBox* typeCombo_;
    QStackedWidget* typePages_;
    QLineEdit* reasonEdit_;
    QComboBox* severityCombo_;
    QLineEdit* triageEdit_;
    QLineEdit* prevDateEdit_;
    QLineEdit* doctorEdit_;
};

} // namespace scapms::gui