#include "gui/PatientPanel.h"
#include "gui/PatientDialog.h"
#include "utils/ClinicException.h"
#include "utils/StringUtils.h"
#include <QAbstractItemView>
#include <QFont>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace scapms::gui {

static QColor rowColor(int priority)
{
    switch (priority) {
        case 1:  return QColor("#ffd9d9");   // Emergency: high priority
        case 2:  return QColor("#d9e8ff");   // Follow-Up: scheduled
        default: return QColor("#e4f5e4");   // General: normal
    }
}

PatientPanel::PatientPanel(ClinicManager& clinic, QWidget* parent)
    : QWidget(parent), clinic_(clinic)
{
    filterEdit_ = new QLineEdit(this);
    filterEdit_->setPlaceholderText("Type a name to filter the table...");
    filterEdit_->setClearButtonEnabled(true);
    filterEdit_->setToolTip("Filter the list as you type (Ctrl+F jumps here).");

    addButton_    = new QPushButton("Add Patient", this);
    editButton_   = new QPushButton("Edit", this);
    deleteButton_ = new QPushButton("Delete", this);
    undoButton_   = new QPushButton("Undo Last Operation", this);
    addButton_->setToolTip("Register a new patient.");
    editButton_->setToolTip("Edit the selected patient (or double-click a row).");
    deleteButton_->setToolTip("Delete the selected patient. This can be undone.");

    auto* legend = new QLabel(
        "<span style='color:#e05555'>&#9632;</span> Emergency (high priority) &nbsp;&nbsp;"
        "<span style='color:#4a86e8'>&#9632;</span> Follow-Up (scheduled) &nbsp;&nbsp;"
        "<span style='color:#4caf50'>&#9632;</span> General (normal)", this);

    table_ = new QTableWidget(0, 6, this);
    table_->setHorizontalHeaderLabels({"ID", "Name", "Age", "Category", "Contact", "Details"});
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->verticalHeader()->setVisible(false);
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->setColumnWidth(0, 60);
    table_->setColumnWidth(1, 190);
    table_->setColumnWidth(2, 55);
    table_->setColumnWidth(3, 100);
    table_->setColumnWidth(4, 130);

    countLabel_ = new QLabel(this);

    // --- Layouts: a horizontal toolbar inside a vertical page layout ---
    auto* toolbar = new QHBoxLayout;
    toolbar->addWidget(new QLabel("Filter:", this));
    toolbar->addWidget(filterEdit_, 1);
    toolbar->addSpacing(16);
    toolbar->addWidget(addButton_);
    toolbar->addWidget(editButton_);
    toolbar->addWidget(deleteButton_);
    toolbar->addWidget(undoButton_);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(toolbar);
    layout->addWidget(legend);
    layout->addWidget(table_, 1);
    layout->addWidget(countLabel_);

    // --- Events ---
    connect(addButton_,    &QPushButton::clicked, this, &PatientPanel::onAdd);
    connect(editButton_,   &QPushButton::clicked, this, &PatientPanel::onEdit);
    connect(deleteButton_, &QPushButton::clicked, this, &PatientPanel::onDelete);
    connect(undoButton_,   &QPushButton::clicked, this, &PatientPanel::undoRequested);
    connect(filterEdit_,   &QLineEdit::textChanged, this, &PatientPanel::onFilterChanged);
    connect(table_, &QTableWidget::itemSelectionChanged, this, &PatientPanel::onSelectionChanged);
    connect(table_, &QTableWidget::cellDoubleClicked, this, [this](int, int) { onEdit(); });

    refresh();
}

void PatientPanel::focusFilter()
{
    filterEdit_->setFocus();
    filterEdit_->selectAll();
}

int PatientPanel::selectedPatientId() const
{
    const auto items = table_->selectedItems();
    if (items.isEmpty()) return -1;
    QTableWidgetItem* idItem = table_->item(items.first()->row(), 0);
    return idItem ? idItem->data(Qt::UserRole).toInt() : -1;
}

void PatientPanel::refresh()
{
    const int keepId = selectedPatientId();
    const std::string filter = filterEdit_->text().trimmed().toStdString();

    table_->setRowCount(0);
    for (const auto& p : clinic_.database().patients()) {
        if (!filter.empty() && !containsIgnoreCase(p->name(), filter)) continue;

        const int row = table_->rowCount();
        table_->insertRow(row);

        const QStringList cells = {
            QString::number(p->id()),
            QString::fromStdString(p->name()),
            QString::number(p->age()),
            QString::fromStdString(p->category()),
            QString::fromStdString(p->contact()),
            QString::fromStdString(p->describe())
        };
        for (int c = 0; c < cells.size(); ++c) {
            auto* item = new QTableWidgetItem(cells[c]);
            item->setBackground(rowColor(p->priority()));
            item->setForeground(QColor("#1b1b1b"));
            if (p->priority() == 1) {                    // emergency rows in bold
                QFont font = item->font();
                font.setBold(true);
                item->setFont(font);
            }
            table_->setItem(row, c, item);
        }
        table_->item(row, 0)->setData(Qt::UserRole, p->id());
        if (p->id() == keepId) table_->selectRow(row);
    }

    const bool canUndo = clinic_.canUndo();
    undoButton_->setEnabled(canUndo);
    undoButton_->setToolTip(canUndo
        ? "Undo: " + QString::fromStdString(clinic_.undoDescription())
        : QString("Nothing to undo."));

    countLabel_->setText(QString("Showing %1 of %2 patients")
                             .arg(table_->rowCount())
                             .arg(static_cast<int>(clinic_.database().size())));
    onSelectionChanged();
}

void PatientPanel::onSelectionChanged()
{
    const bool selected = selectedPatientId() >= 0;
    editButton_->setEnabled(selected);
    deleteButton_->setEnabled(selected);
}

void PatientPanel::onFilterChanged(const QString&)
{
    refresh();
}

void PatientPanel::onAdd()
{
    PatientDialog dialog(clinic_, nullptr, this);
    if (dialog.exec() != QDialog::Accepted) return;

    try {
        clinic_.registerPatient(PatientDialog::makePatient(dialog.form()));
        emit dataChanged();
    } catch (const ClinicException& e) {
        QMessageBox::warning(this, "Could not add patient",
                             QString("Error: ") + QString::fromStdString(e.what()));
    }
}

void PatientPanel::onEdit()
{
    const int id = selectedPatientId();
    if (id < 0) return;

    PatientRecord* patient = clinic_.findPatient(id);
    if (!patient) return;

    PatientDialog dialog(clinic_, patient, this);
    if (dialog.exec() != QDialog::Accepted) return;

    const PatientForm form = dialog.form();
    try {
        clinic_.editPatient(id, [&form](PatientRecord& p) { PatientDialog::applyForm(p, form); });
        emit dataChanged();
    } catch (const ClinicException& e) {
        QMessageBox::warning(this, "Could not edit patient",
                             QString("Error: ") + QString::fromStdString(e.what()));
    }
}

void PatientPanel::onDelete()
{
    const int id = selectedPatientId();
    if (id < 0) return;

    PatientRecord* patient = clinic_.findPatient(id);
    if (!patient) return;

    const auto answer = QMessageBox::question(this, "Delete patient",
        QString("Delete %1 (ID %2)?\nYou can undo this with \"Undo Last Operation\".")
            .arg(QString::fromStdString(patient->name())).arg(id));
    if (answer != QMessageBox::Yes) return;

    try {
        clinic_.deletePatient(id);
        emit dataChanged();
    } catch (const ClinicException& e) {
        QMessageBox::warning(this, "Could not delete patient", QString::fromStdString(e.what()));
    }
}

} // namespace scapms::gui