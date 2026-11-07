#include "gui/ReportPanel.h"
#include "controller/ReportGenerator.h"
#include "utils/ClinicException.h"
#include <QFile>
#include <QFileDialog>
#include <QFont>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTextStream>
#include <QVBoxLayout>

namespace carepilot::gui {

ReportPanel::ReportPanel(ClinicManager& clinic, QWidget* parent)
    : QWidget(parent), clinic_(clinic)
{
    auto* attendedButton = new QPushButton("Most Frequently Attended Patients", this);
    auto* pendingButton  = new QPushButton("Patients with Pending Appointments", this);
    auto* categoryButton = new QPushButton("Patient Category Distribution", this);
    auto* dailyButton    = new QPushButton("Daily Appointment Summary", this);
    auto* allButton      = new QPushButton("Generate All Reports", this);
    auto* exportButton   = new QPushButton("Export to File...", this);

    attendedButton->setToolTip("Patients ranked by completed appointments.");
    pendingButton->setToolTip("Patients who still have appointments to attend.");
    categoryButton->setToolTip("How many General, Emergency and Follow-Up patients there are.");
    dailyButton->setToolTip("Total, completed, pending and cancelled appointments on the date below.");
    exportButton->setToolTip("Save the report on screen as a text file.");

    dateEdit_ = new QLineEdit(QString::fromStdString(ReportGenerator::todayDate()), this);
    dateEdit_->setPlaceholderText("YYYY-MM-DD");
    dateEdit_->setToolTip("Date for the daily summary, e.g. 2026-11-07.");

    output_ = new QPlainTextEdit(this);
    output_->setReadOnly(true);
    output_->setPlaceholderText("Choose a report on the left.");
    QFont mono("Consolas", 11);
    mono.setStyleHint(QFont::Monospace);
    output_->setFont(mono);

    auto* box = new QGroupBox("Reports", this);
    box->setMaximumWidth(300);
    auto* boxLayout = new QVBoxLayout(box);
    boxLayout->addWidget(attendedButton);
    boxLayout->addWidget(pendingButton);
    boxLayout->addWidget(categoryButton);
    boxLayout->addSpacing(8);
    boxLayout->addWidget(new QLabel("Date for the daily summary:", this));
    boxLayout->addWidget(dateEdit_);
    boxLayout->addWidget(dailyButton);
    boxLayout->addSpacing(8);
    boxLayout->addWidget(allButton);
    boxLayout->addWidget(exportButton);
    boxLayout->addStretch(1);

    auto* layout = new QHBoxLayout(this);
    layout->addWidget(box);
    layout->addWidget(output_, 1);

    connect(attendedButton, &QPushButton::clicked, this, [this] { showReport(0); });
    connect(pendingButton,  &QPushButton::clicked, this, [this] { showReport(1); });
    connect(categoryButton, &QPushButton::clicked, this, [this] { showReport(2); });
    connect(dailyButton,    &QPushButton::clicked, this, [this] { showReport(3); });
    connect(dateEdit_, &QLineEdit::returnPressed,  this, [this] { showReport(3); });
    connect(allButton,      &QPushButton::clicked, this, [this] { showReport(4); });
    connect(exportButton,   &QPushButton::clicked, this, &ReportPanel::onExport);
}

std::string ReportPanel::build(int which)
{
    ClinicDatabase& db = clinic_.database();
    const std::string date = dateEdit_->text().trimmed().toStdString();
    switch (which) {
        case 0: return ReportGenerator::mostFrequentlyAttended(db);
        case 1: return ReportGenerator::pendingAppointments(db);
        case 2: return ReportGenerator::categoryDistribution(db);
        case 3: return ReportGenerator::dailySummary(db, date);
        default:
            return ReportGenerator::categoryDistribution(db) + "\n" +
                   ReportGenerator::mostFrequentlyAttended(db) + "\n" +
                   ReportGenerator::pendingAppointments(db) + "\n" +
                   ReportGenerator::dailySummary(db, date);
    }
}

void ReportPanel::showReport(int which)
{
    try {
        output_->setPlainText(QString::fromStdString(build(which)));
        current_ = which;
    } catch (const ClinicException& e) {
        QMessageBox::warning(this, "Could not generate report",
                             QString("Error: ") + QString::fromStdString(e.what()));
    } catch (...) {
        QMessageBox::critical(this, "Unexpected error", "An unknown error occurred.");
    }
}

void ReportPanel::refresh()
{
    if (current_ < 0) return;
    try {
        output_->setPlainText(QString::fromStdString(build(current_)));
    } catch (const ClinicException&) {
        output_->setPlainText("Enter a valid date (YYYY-MM-DD) to see this report.");
    }
}

void ReportPanel::onExport()
{
    try {
        if (output_->toPlainText().isEmpty())
            throw ClinicException("Generate a report first, then export it.");

        const QString path = QFileDialog::getSaveFileName(this, "Export report",
            "carepilot_report.txt", "Text files (*.txt);;All files (*)");
        if (path.isEmpty()) return;

        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            throw FileException("Cannot write to file: " + path.toStdString());
        QTextStream out(&file);
        out << output_->toPlainText() << "\n";
    } catch (const ClinicException& e) {
        QMessageBox::warning(this, "Export failed",
                             QString("Error: ") + QString::fromStdString(e.what()));
    } catch (...) {
        QMessageBox::critical(this, "Unexpected error", "An unknown error occurred.");
    }
}

} // namespace carepilot::gui