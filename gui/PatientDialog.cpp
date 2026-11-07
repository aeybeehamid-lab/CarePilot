#include "gui/PatientDialog.h"
#include "model/EmergencyPatient.h"
#include "model/FollowUpPatient.h"
#include "model/GeneralPatient.h"
#include "utils/ClinicException.h"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace carepilot::gui {

static int parseWholeNumber(const QString& text, const QString& label)
{
    bool ok = false;
    int value = text.trimmed().toInt(&ok);
    if (!ok)
        throw InvalidPatientException((label + " must be a whole number.").toStdString());
    return value;
}

PatientDialog::PatientDialog(const ClinicManager& clinic, const PatientRecord* existing,
                             QWidget* parent)
    : QDialog(parent), clinic_(clinic), editing_(existing != nullptr)
{
    setWindowTitle(editing_ ? "Edit Patient" : "Add Patient");
    setMinimumWidth(430);

    idEdit_      = new QLineEdit(this);
    nameEdit_    = new QLineEdit(this);
    ageEdit_     = new QLineEdit(this);
    contactEdit_ = new QLineEdit(this);
    typeCombo_   = new QComboBox(this);
    typeCombo_->addItems({"General", "Emergency", "Follow-Up"});

    idEdit_->setToolTip("Unique patient ID. A free ID is suggested automatically.");
    nameEdit_->setToolTip("Patient's full name.");
    ageEdit_->setToolTip("Age in years (0 to 120).");
    contactEdit_->setToolTip("Phone number or email address.");
    typeCombo_->setToolTip("Choosing a type changes the extra fields shown below.");

    // --- One page of extra fields per patient type ---
    typePages_ = new QStackedWidget(this);

    auto* generalPage = new QWidget;
    auto* generalForm = new QFormLayout(generalPage);
    reasonEdit_ = new QLineEdit;
    reasonEdit_->setToolTip("Why the patient is visiting the clinic.");
    generalForm->addRow("Reason for visit:", reasonEdit_);

    auto* emergencyPage = new QWidget;
    auto* emergencyForm = new QFormLayout(emergencyPage);
    severityCombo_ = new QComboBox;
    severityCombo_->addItems({"1", "2", "3", "4", "5"});
    severityCombo_->setToolTip("1 = mild, 5 = life-threatening. 4 and 5 are flagged CRITICAL.");
    triageEdit_ = new QLineEdit;
    emergencyForm->addRow("Severity (1-5):", severityCombo_);
    emergencyForm->addRow("Triage note:", triageEdit_);

    auto* followPage = new QWidget;
    auto* followForm = new QFormLayout(followPage);
    prevDateEdit_ = new QLineEdit;
    prevDateEdit_->setPlaceholderText("YYYY-MM-DD");
    prevDateEdit_->setToolTip("Date of the previous visit, e.g. 2026-10-20.");
    doctorEdit_ = new QLineEdit;
    followForm->addRow("Previous visit:", prevDateEdit_);
    followForm->addRow("Doctor:", doctorEdit_);

    typePages_->addWidget(generalPage);     // index 0
    typePages_->addWidget(emergencyPage);   // index 1
    typePages_->addWidget(followPage);      // index 2

    // --- Layouts ---
    auto* commonForm = new QFormLayout;
    commonForm->addRow("Patient ID:", idEdit_);
    commonForm->addRow("Full name:", nameEdit_);
    commonForm->addRow("Age:", ageEdit_);
    commonForm->addRow("Contact:", contactEdit_);
    commonForm->addRow("Patient type:", typeCombo_);

    auto* group = new QGroupBox("Details for this patient type", this);
    auto* groupLayout = new QVBoxLayout(group);
    groupLayout->addWidget(typePages_);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &PatientDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &PatientDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(commonForm);
    layout->addWidget(group);
    layout->addWidget(buttons);

    connect(typeCombo_, &QComboBox::currentIndexChanged, this, &PatientDialog::onTypeChanged);

    if (existing) {
        fillFrom(*existing);
        idEdit_->setReadOnly(true);      // the ID and type of a patient cannot change
        typeCombo_->setEnabled(false);
    } else {
        idEdit_->setText(QString::number(clinic_.suggestPatientId()));
    }
    onTypeChanged(typeCombo_->currentIndex());
}

void PatientDialog::onTypeChanged(int index)
{
    typePages_->setCurrentIndex(index);
}

void PatientDialog::fillFrom(const PatientRecord& p)
{
    idEdit_->setText(QString::number(p.id()));
    nameEdit_->setText(QString::fromStdString(p.name()));
    ageEdit_->setText(QString::number(p.age()));
    contactEdit_->setText(QString::fromStdString(p.contact()));

    if (auto* g = dynamic_cast<const GeneralPatient*>(&p)) {
        typeCombo_->setCurrentIndex(0);
        reasonEdit_->setText(QString::fromStdString(g->reasonForVisit()));
    } else if (auto* e = dynamic_cast<const EmergencyPatient*>(&p)) {
        typeCombo_->setCurrentIndex(1);
        severityCombo_->setCurrentIndex(e->severity() - 1);
        triageEdit_->setText(QString::fromStdString(e->triageNote()));
    } else if (auto* f = dynamic_cast<const FollowUpPatient*>(&p)) {
        typeCombo_->setCurrentIndex(2);
        prevDateEdit_->setText(QString::fromStdString(f->previousVisitDate()));
        doctorEdit_->setText(QString::fromStdString(f->doctorName()));
    }
}

PatientForm PatientDialog::readForm() const
{
    PatientForm f;
    f.id                = parseWholeNumber(idEdit_->text(), "Patient ID");
    f.name              = nameEdit_->text().trimmed().toStdString();
    f.age               = parseWholeNumber(ageEdit_->text(), "Patient age");
    f.contact           = contactEdit_->text().trimmed().toStdString();
    f.type              = typeCombo_->currentIndex();
    f.reason            = reasonEdit_->text().trimmed().toStdString();
    f.severity          = severityCombo_->currentIndex() + 1;
    f.triageNote        = triageEdit_->text().trimmed().toStdString();
    f.previousVisitDate = prevDateEdit_->text().trimmed().toStdString();
    f.doctorName        = doctorEdit_->text().trimmed().toStdString();
    return f;
}

void PatientDialog::accept()
{
    try {
        PatientForm f = readForm();
        std::unique_ptr<PatientRecord> check = makePatient(f);   // validates every field
        if (!editing_ && clinic_.database().containsId(f.id))
            throw DuplicateIdException("Patient ID already exists.");
        form_ = f;
    }
    catch (const ClinicException& e) {
        QMessageBox::warning(this, "Invalid input",
                             QString("Error: ") + QString::fromStdString(e.what()));
        return;                                                  // keep the dialog open
    }
    catch (const std::exception& e) {
        QMessageBox::critical(this, "Unexpected error", QString::fromStdString(e.what()));
        return;
    }
    catch (...) {
        QMessageBox::critical(this, "Unexpected error", "An unknown error occurred.");
        return;
    }
    QDialog::accept();
}

const PatientForm& PatientDialog::form() const { return form_; }

std::unique_ptr<PatientRecord> PatientDialog::makePatient(const PatientForm& f)
{
    switch (f.type) {
        case 0: return std::make_unique<GeneralPatient>(f.id, f.name, f.age, f.contact, f.reason);
        case 1: return std::make_unique<EmergencyPatient>(f.id, f.name, f.age, f.contact,
                                                          f.severity, f.triageNote);
        case 2: return std::make_unique<FollowUpPatient>(f.id, f.name, f.age, f.contact,
                                                         f.previousVisitDate, f.doctorName);
    }
    throw InvalidPatientException("Unknown patient type.");
}

void PatientDialog::applyForm(PatientRecord& patient, const PatientForm& f)
{
    patient.setName(f.name);
    patient.setAge(f.age);
    patient.setContact(f.contact);

    if (auto* g = dynamic_cast<GeneralPatient*>(&patient)) {
        g->setReasonForVisit(f.reason);
    } else if (auto* e = dynamic_cast<EmergencyPatient*>(&patient)) {
        e->setSeverity(f.severity);
        e->setTriageNote(f.triageNote);
    } else if (auto* fu = dynamic_cast<FollowUpPatient*>(&patient)) {
        fu->setPreviousVisitDate(f.previousVisitDate);
        fu->setDoctorName(f.doctorName);
    }
}

} // namespace carepilot::gui