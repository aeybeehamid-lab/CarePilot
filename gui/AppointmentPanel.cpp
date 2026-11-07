#include "gui/AppointmentPanel.h"
#include "controller/ReportGenerator.h"
#include "model/Appointable.h"
#include "utils/ClinicException.h"
#include <QAbstractItemView>
#include <QComboBox>
#include <QFont>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <string>
#include <vector>

namespace carepilot::gui {

struct AppointmentRow {
    int appointmentId;
    int patientId;
    std::string patientName;
    std::string date;
    std::string time;
    std::string reason;
    AppointmentStatus status;
};

AppointmentPanel::AppointmentPanel(ClinicManager& clinic, QWidget* parent)
    : QWidget(parent), clinic_(clinic)
{
    // ---- Scheduling form (grid layout) ----
    patientCombo_ = new QComboBox(this);
    patientCombo_->setToolTip("Only General and Follow-Up patients can book appointments.\n"
                              "Emergency patients go to the Waiting Queue.");
    dateEdit_ = new QLineEdit(QString::fromStdString(ReportGenerator::todayDate()), this);
    dateEdit_->setPlaceholderText("YYYY-MM-DD");
    dateEdit_->setToolTip("Appointment date, e.g. 2026-11-20.");
    timeEdit_ = new QLineEdit("09:00", this);
    timeEdit_->setPlaceholderText("HH:MM");
    timeEdit_->setToolTip("24-hour time, e.g. 14:30.");
    reasonEdit_ = new QLineEdit(this);
    reasonEdit_->setPlaceholderText("Reason for the appointment");
    scheduleButton_ = new QPushButton("Schedule Appointment", this);
    scheduleButton_->setToolTip("Book this appointment for the selected patient.");

    auto* formBox = new QGroupBox("Schedule a new appointment", this);
    auto* grid = new QGridLayout(formBox);
    grid->addWidget(new QLabel("Patient:"), 0, 0);
    grid->addWidget(patientCombo_, 0, 1, 1, 3);
    grid->addWidget(new QLabel("Date:"), 1, 0);
    grid->addWidget(dateEdit_, 1, 1);
    grid->addWidget(new QLabel("Time:"), 1, 2);
    grid->addWidget(timeEdit_, 1, 3);
    grid->addWidget(new QLabel("Reason:"), 2, 0);
    grid->addWidget(reasonEdit_, 2, 1, 1, 3);
    grid->addWidget(scheduleButton_, 3, 3);
    grid->setColumnStretch(1, 1);
    grid->setColumnStretch(3, 1);

    // ---- Appointment list ----
    statusFilter_ = new QComboBox(this);
    statusFilter_->addItems({"All", "Pending", "Completed", "Cancelled"});
    statusFilter_->setToolTip("Show only appointments with this status.");
    cancelButton_   = new QPushButton("Cancel Appointment", this);
    completeButton_ = new QPushButton("Mark Completed", this);
    cancelButton_->setToolTip("Cancel the selected pending appointment (can be undone).");
    completeButton_->setToolTip("Mark the selected pending appointment as completed.");

    table_ = new QTableWidget(0, 6, this);
    table_->setHorizontalHeaderLabels({"Appt ID", "Patient", "Date", "Time", "Reason", "Status"});
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->verticalHeader()->setVisible(false);
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->setColumnWidth(0, 70);
    table_->setColumnWidth(1, 190);
    table_->setColumnWidth(2, 100);
    table_->setColumnWidth(3, 70);
    table_->setColumnWidth(4, 220);

    countLabel_ = new QLabel(this);

    auto* toolbar = new QHBoxLayout;
    toolbar->addWidget(new QLabel("Show:", this));
    toolbar->addWidget(statusFilter_);
    toolbar->addStretch(1);
    toolbar->addWidget(cancelButton_);
    toolbar->addWidget(completeButton_);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(formBox);
    layout->addLayout(toolbar);
    layout->addWidget(table_, 1);
    layout->addWidget(countLabel_);

    // ---- Events ----
    connect(scheduleButton_, &QPushButton::clicked, this, &AppointmentPanel::onSchedule);
    connect(reasonEdit_, &QLineEdit::returnPressed, this, &AppointmentPanel::onSchedule);
    connect(cancelButton_, &QPushButton::clicked, this, &AppointmentPanel::onCancel);
    connect(completeButton_, &QPushButton::clicked, this, &AppointmentPanel::onComplete);
    connect(statusFilter_, &QComboBox::currentIndexChanged, this, [this](int) { refreshTable(); });
    connect(table_, &QTableWidget::itemSelectionChanged, this, &AppointmentPanel::onSelectionChanged);

    refresh();
}

void AppointmentPanel::refresh()
{
    refreshPatientCombo();
    refreshTable();
}

void AppointmentPanel::refreshPatientCombo()
{
    const int keepId = patientCombo_->currentData().toInt();
    patientCombo_->clear();
    for (const auto& p : clinic_.database().patients()) {
        if (!dynamic_cast<const Appointable*>(p.get())) continue;   // runtime interface check
        patientCombo_->addItem(QString("%1 - %2 (%3)")
                                   .arg(p->id())
                                   .arg(QString::fromStdString(p->name()))
                                   .arg(QString::fromStdString(p->category())),
                               p->id());
    }
    const int index = patientCombo_->findData(keepId);
    if (index >= 0) patientCombo_->setCurrentIndex(index);
}

void AppointmentPanel::refreshTable()
{
    const int keepId = selectedAppointmentId();
    const int filter = statusFilter_->currentIndex();   // 0 all, 1 pending, 2 completed, 3 cancelled

    std::vector<AppointmentRow> rows;
    for (const auto& p : clinic_.database().patients()) {
        for (const Appointment& a : p->appointments()) {
            if (filter == 1 && a.status() != AppointmentStatus::Pending)   continue;
            if (filter == 2 && a.status() != AppointmentStatus::Completed) continue;
            if (filter == 3 && a.status() != AppointmentStatus::Cancelled) continue;
            rows.push_back({a.id(), p->id(), p->name(), a.date(), a.time(), a.reason(), a.status()});
        }
    }

    // Insertion sort by date, then time (earliest first).
    for (size_t i = 1; i < rows.size(); ++i) {
        AppointmentRow key = rows[i];
        const std::string keyText = key.date + " " + key.time;
        size_t j = i;
        while (j > 0 && keyText < rows[j - 1].date + " " + rows[j - 1].time) {
            rows[j] = rows[j - 1];
            --j;
        }
        rows[j] = key;
    }

    table_->setRowCount(0);
    for (const AppointmentRow& r : rows) {
        const int row = table_->rowCount();
        table_->insertRow(row);

        QColor background("#fff3cd");                  // Pending: amber
        if (r.status == AppointmentStatus::Completed) background = QColor("#d4edda");
        if (r.status == AppointmentStatus::Cancelled) background = QColor("#e2e3e5");

        const QStringList cells = {
            QString::number(r.appointmentId),
            QString::fromStdString(r.patientName),
            QString::fromStdString(r.date),
            QString::fromStdString(r.time),
            QString::fromStdString(r.reason),
            QString::fromStdString(Appointment::statusToText(r.status))
        };
        for (int c = 0; c < cells.size(); ++c) {
            auto* item = new QTableWidgetItem(cells[c]);
            item->setBackground(background);
            item->setForeground(QColor("#1b1b1b"));
            if (r.status == AppointmentStatus::Cancelled) {
                QFont font = item->font();
                font.setItalic(true);
                font.setStrikeOut(c != 5);              // strike through everything but the status
                item->setFont(font);
            }
            table_->setItem(row, c, item);
        }
        table_->item(row, 0)->setData(Qt::UserRole, r.appointmentId);
        table_->item(row, 0)->setData(Qt::UserRole + 1, r.patientId);
        if (r.appointmentId == keepId) table_->selectRow(row);
    }

    countLabel_->setText(QString("Showing %1 appointment(s)").arg(table_->rowCount()));
    onSelectionChanged();
}

int AppointmentPanel::selectedAppointmentId() const
{
    const auto items = table_->selectedItems();
    if (items.isEmpty()) return -1;
    QTableWidgetItem* item = table_->item(items.first()->row(), 0);
    return item ? item->data(Qt::UserRole).toInt() : -1;
}

int AppointmentPanel::selectedPatientId() const
{
    const auto items = table_->selectedItems();
    if (items.isEmpty()) return -1;
    QTableWidgetItem* item = table_->item(items.first()->row(), 0);
    return item ? item->data(Qt::UserRole + 1).toInt() : -1;
}

void AppointmentPanel::onSelectionChanged()
{
    bool pending = false;
    const auto items = table_->selectedItems();
    if (!items.isEmpty()) {
        QTableWidgetItem* status = table_->item(items.first()->row(), 5);
        pending = status && status->text() == "Pending";
    }
    cancelButton_->setEnabled(pending);
    completeButton_->setEnabled(pending);
}

void AppointmentPanel::onSchedule()
{
    try {
        if (patientCombo_->count() == 0)
            throw InvalidAppointmentException(
                "No patient can book appointments yet. Register a General or Follow-Up patient first.");

        clinic_.scheduleAppointment(patientCombo_->currentData().toInt(),
                                    dateEdit_->text().trimmed().toStdString(),
                                    timeEdit_->text().trimmed().toStdString(),
                                    reasonEdit_->text().trimmed().toStdString());
        reasonEdit_->clear();
        emit dataChanged();
    } catch (const ClinicException& e) {
        QMessageBox::warning(this, "Could not schedule appointment",
                             QString("Error: ") + QString::fromStdString(e.what()));
    } catch (...) {
        QMessageBox::critical(this, "Unexpected error", "An unknown error occurred.");
    }
}

void AppointmentPanel::onCancel()
{
    const int appointmentId = selectedAppointmentId();
    const int patientId = selectedPatientId();
    if (appointmentId < 0) return;
    try {
        clinic_.cancelAppointment(patientId, appointmentId);
        emit dataChanged();
    } catch (const ClinicException& e) {
        QMessageBox::warning(this, "Could not cancel appointment",
                             QString("Error: ") + QString::fromStdString(e.what()));
    }
}

void AppointmentPanel::onComplete()
{
    const int appointmentId = selectedAppointmentId();
    const int patientId = selectedPatientId();
    if (appointmentId < 0) return;
    try {
        clinic_.completeAppointment(patientId, appointmentId);
        emit dataChanged();
    } catch (const ClinicException& e) {
        QMessageBox::warning(this, "Could not update appointment",
                             QString("Error: ") + QString::fromStdString(e.what()));
    }
}

} // namespace carepilot::gui