#include "gui/QueuePanel.h"
#include "controller/SearchEngine.h"
#include "gui/GuiHelpers.h"
#include "utils/ClinicException.h"
#include <QAbstractItemView>
#include <QComboBox>
#include <QFont>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <cmath>
#include <ctime>
#include <vector>

namespace carepilot::gui {

QueuePanel::QueuePanel(ClinicManager& clinic, QWidget* parent)
    : QWidget(parent), clinic_(clinic)
{
    patientCombo_ = new QComboBox(this);
    patientCombo_->setMinimumWidth(280);
    patientCombo_->setToolTip("Choose the patient who has arrived.");
    addButton_ = new QPushButton("Add to Queue", this);
    addButton_->setToolTip("Put the chosen patient at the back of the waiting queue.");
    callNextButton_ = new QPushButton("Call Next Patient", this);
    callNextButton_->setToolTip("Serve the patient who has waited longest (first in, first out).");

    QFont big = font();
    big.setPointSize(big.pointSize() + 4);
    big.setBold(true);
    nowServing_ = new QLabel("Now serving: nobody yet", this);
    nowServing_->setFont(big);

    summary_ = new QLabel(this);

    table_ = new QTableWidget(0, 5, this);
    table_->setHorizontalHeaderLabels({"Position", "ID", "Name", "Category", "Waiting (mm:ss)"});
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::NoSelection);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->verticalHeader()->setVisible(false);
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->setColumnWidth(0, 80);
    table_->setColumnWidth(1, 60);
    table_->setColumnWidth(2, 220);
    table_->setColumnWidth(3, 110);

    auto* controls = new QHBoxLayout;
    controls->addWidget(new QLabel("Patient:", this));
    controls->addWidget(patientCombo_);
    controls->addWidget(addButton_);
    controls->addStretch(1);
    controls->addWidget(callNextButton_);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(controls);
    layout->addWidget(nowServing_);
    layout->addWidget(table_, 1);
    layout->addWidget(summary_);

    connect(addButton_, &QPushButton::clicked, this, &QueuePanel::onAdd);
    connect(callNextButton_, &QPushButton::clicked, this, &QueuePanel::onCallNext);

    refresh();
}

void QueuePanel::refresh()
{
    const int keepId = patientCombo_->currentData().toInt();
    patientCombo_->clear();
    for (const auto& p : clinic_.database().patients())
        patientCombo_->addItem(QString("%1 - %2 (%3)")
                                   .arg(p->id())
                                   .arg(QString::fromStdString(p->name()))
                                   .arg(QString::fromStdString(p->category())),
                               p->id());
    const int index = patientCombo_->findData(keepId);
    if (index >= 0) patientCombo_->setCurrentIndex(index);
    refreshTable();
}

void QueuePanel::refreshTable()
{
    const std::vector<QueueEntry> waiting = clinic_.waitingList();
    const std::time_t now = std::time(nullptr);

    table_->setRowCount(0);
    int position = 1;
    for (const QueueEntry& entry : waiting) {
        // Look the patient up without touching the "recently used" cache.
        const int index = SearchEngine::findById(clinic_.database(), entry.patientId);
        if (index < 0) continue;
        const PatientRecord& p = *clinic_.database().patients()[static_cast<size_t>(index)];
        const long waited = static_cast<long>(std::difftime(now, entry.joinedAt));

        const int row = table_->rowCount();
        table_->insertRow(row);
        const QStringList cells = {
            QString::number(position++),
            QString::number(p.id()),
            QString::fromStdString(p.name()),
            QString::fromStdString(p.category()),
            formatWait(waited)
        };
        for (int c = 0; c < cells.size(); ++c) {
            auto* item = new QTableWidgetItem(cells[c]);
            item->setBackground(priorityColor(p.priority()));
            item->setForeground(QColor("#1b1b1b"));
            if (c == 4 && waited >= WAIT_ALERT_SECONDS) {       // long wait: highlight in red
                item->setBackground(QColor("#ef9a9a"));
                QFont f = item->font();
                f.setBold(true);
                item->setFont(f);
            }
            table_->setItem(row, c, item);
        }
    }

    summary_->setText(QString("%1 patient(s) waiting   |   Longest wait: %2")
                          .arg(static_cast<int>(clinic_.queueSize()))
                          .arg(formatWait(clinic_.longestWaitSeconds())));
    callNextButton_->setEnabled(clinic_.queueSize() > 0);
}

void QueuePanel::onAdd()
{
    try {
        if (patientCombo_->count() == 0)
            throw ClinicException("There are no registered patients to add.");
        clinic_.addToQueue(patientCombo_->currentData().toInt());
        emit queueChanged();
    } catch (const ClinicException& e) {
        QMessageBox::warning(this, "Could not add to queue",
                             QString("Error: ") + QString::fromStdString(e.what()));
    } catch (...) {
        QMessageBox::critical(this, "Unexpected error", "An unknown error occurred.");
    }
}

void QueuePanel::onCallNext()
{
    try {
        const int id = clinic_.callNextPatient();
        const PatientRecord* p = clinic_.findPatient(id);
        const QString text = p ? QString("%1 (ID %2, %3)")
                                     .arg(QString::fromStdString(p->name()))
                                     .arg(id)
                                     .arg(QString::fromStdString(p->category()))
                               : QString("ID %1").arg(id);
        nowServing_->setText("Now serving: " + text);
        emit queueChanged();
        QMessageBox::information(this, "Next patient", "Please call in: " + text);
    } catch (const ClinicException& e) {
        QMessageBox::information(this, "Waiting queue", QString::fromStdString(e.what()));
    }
}

} // namespace carepilot::gui