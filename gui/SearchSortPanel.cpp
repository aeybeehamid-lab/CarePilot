#include "gui/SearchSortPanel.h"
#include "controller/SearchEngine.h"
#include "controller/SortEngine.h"
#include "gui/GuiHelpers.h"
#include "utils/ClinicException.h"
#include <QAbstractItemView>
#include <QComboBox>
#include <QFont>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <cctype>
#include <string>

namespace carepilot::gui {

static SortAlgorithm algorithmFromIndex(int index)
{
    switch (index) {
        case 0:  return SortAlgorithm::Selection;
        case 1:  return SortAlgorithm::Insertion;
        default: return SortAlgorithm::Merge;
    }
}

static SortField fieldFromIndex(int index)
{
    switch (index) {
        case 0:  return SortField::Id;
        case 1:  return SortField::Name;
        default: return SortField::Age;
    }
}

// "followup", "Follow Up" and "follow-up" all mean the same category.
static std::string canonicalCategory(const std::string& text)
{
    std::string key;
    for (char c : text)
        if (c != '-' && c != ' ')
            key += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    if (key == "general")   return "General";
    if (key == "emergency") return "Emergency";
    if (key == "followup")  return "Follow-Up";
    throw InvalidSearchException("Category must be General, Emergency or Follow-Up.");
}

SearchSortPanel::SearchSortPanel(ClinicManager& clinic, QWidget* parent)
    : QWidget(parent), clinic_(clinic)
{
    // ---------- Search group ----------
    fieldCombo_ = new QComboBox(this);
    fieldCombo_->addItems({"Patient ID", "Patient Name", "Category"});
    fieldCombo_->setToolTip("Choose which field to search.");
    algoCombo_ = new QComboBox(this);
    algoCombo_->setToolTip("Choose the searching algorithm.");
    searchEdit_ = new QLineEdit(this);
    searchEdit_->setClearButtonEnabled(true);
    searchButton_ = new QPushButton("Search", this);
    searchButton_->setToolTip("Run the search (or press Enter in the keyword box).");
    showAllButton_ = new QPushButton("Show All", this);
    showAllButton_->setToolTip("Clear the search and show every patient.");

    auto* searchBox = new QGroupBox("Search", this);
    auto* searchForm = new QFormLayout(searchBox);
    searchForm->addRow("Search by:", fieldCombo_);
    searchForm->addRow("Search algorithm:", algoCombo_);
    auto* keyRow = new QHBoxLayout;
    keyRow->addWidget(searchEdit_, 1);
    keyRow->addWidget(searchButton_);
    keyRow->addWidget(showAllButton_);
    searchForm->addRow("Keyword:", keyRow);

    // ---------- Sort group ----------
    sortAlgoCombo_ = new QComboBox(this);
    sortAlgoCombo_->addItems({"Selection Sort", "Insertion Sort", "Merge Sort"});
    sortAlgoCombo_->setToolTip("Choose the sorting algorithm.");
    sortFieldCombo_ = new QComboBox(this);
    sortFieldCombo_->addItems({"Patient ID", "Patient Name", "Age"});
    sortFieldCombo_->setToolTip("Choose the field to sort by.");
    sortButton_ = new QPushButton("SORT", this);
    sortButton_->setToolTip("Sort the patient records with the chosen algorithm.");

    explanationLabel_ = new QLabel(this);
    explanationLabel_->setWordWrap(true);
    QFont italic = explanationLabel_->font();
    italic.setItalic(true);
    explanationLabel_->setFont(italic);
    sortedLabel_ = new QLabel(this);

    auto* sortBox = new QGroupBox("Sort", this);
    auto* sortForm = new QFormLayout(sortBox);
    sortForm->addRow("Sort algorithm:", sortAlgoCombo_);
    sortForm->addRow("Sort by:", sortFieldCombo_);
    sortForm->addRow("", sortButton_);
    sortForm->addRow("Why this algorithm?", explanationLabel_);
    sortForm->addRow("State:", sortedLabel_);

    // ---------- Results ----------
    resultLabel_ = new QLabel(this);
    resultLabel_->setWordWrap(true);

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

    auto* top = new QHBoxLayout;
    top->addWidget(searchBox, 1);
    top->addWidget(sortBox, 1);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(top);
    layout->addWidget(resultLabel_);
    layout->addWidget(table_, 1);

    onSearchFieldChanged(0);
    onSortAlgorithmChanged(0);

    // ---------- Events ----------
    connect(fieldCombo_, &QComboBox::currentIndexChanged, this, &SearchSortPanel::onSearchFieldChanged);
    connect(sortAlgoCombo_, &QComboBox::currentIndexChanged, this, &SearchSortPanel::onSortAlgorithmChanged);
    connect(searchButton_, &QPushButton::clicked, this, &SearchSortPanel::onSearch);
    connect(searchEdit_, &QLineEdit::returnPressed, this, &SearchSortPanel::onSearch);
    connect(showAllButton_, &QPushButton::clicked, this, &SearchSortPanel::onShowAll);
    connect(sortButton_, &QPushButton::clicked, this, &SearchSortPanel::onSort);

    refresh();
}

void SearchSortPanel::focusSearch()
{
    searchEdit_->setFocus();
    searchEdit_->selectAll();
}

void SearchSortPanel::refresh()
{
    std::vector<const PatientRecord*> all;
    for (const auto& p : clinic_.database().patients()) all.push_back(p.get());
    showPatients(all);
    updateSortedLabel();
    resultLabel_->setText(QString("Showing all %1 patient(s).").arg(static_cast<int>(all.size())));
}

void SearchSortPanel::showPatients(const std::vector<const PatientRecord*>& patients)
{
    table_->setRowCount(0);
    for (const PatientRecord* p : patients) {
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
            item->setBackground(priorityColor(p->priority()));
            item->setForeground(QColor("#1b1b1b"));
            table_->setItem(row, c, item);
        }
    }
}

void SearchSortPanel::updateSortedLabel()
{
    switch (clinic_.database().sortedField()) {
        case SortField::Id:
            sortedLabel_->setText("Sorted by Patient ID (ID search will use Binary Search)");
            break;
        case SortField::Name: sortedLabel_->setText("Sorted by Patient Name"); break;
        case SortField::Age:  sortedLabel_->setText("Sorted by Age"); break;
        case SortField::None: sortedLabel_->setText("Not sorted (original order)"); break;
    }
}

void SearchSortPanel::onSearchFieldChanged(int index)
{
    algoCombo_->clear();
    switch (index) {
        case 0:
            algoCombo_->addItems({"Automatic (based on the data's state)",
                                  "Linear Search",
                                  "Binary Search (list must be sorted by ID)"});
            searchEdit_->setPlaceholderText("Enter a patient ID, e.g. 4");
            break;
        case 1:
            algoCombo_->addItem("Recursive Search (partial name match)");
            searchEdit_->setPlaceholderText("Part of a name, e.g. ham");
            break;
        default:
            algoCombo_->addItem("Linear Scan");
            searchEdit_->setPlaceholderText("General, Emergency or Follow-Up");
            break;
    }
}

void SearchSortPanel::onSortAlgorithmChanged(int index)
{
    explanationLabel_->setText(
        QString::fromStdString(SortEngine::explain(algorithmFromIndex(index))));
}

void SearchSortPanel::onSearch()
{
    try {
        ClinicDatabase& db = clinic_.database();
        const QString raw = searchEdit_->text().trimmed();
        if (raw.isEmpty())
            throw InvalidSearchException("Search keyword cannot be empty.");

        const int field = fieldCombo_->currentIndex();
        const int algo = algoCombo_->currentIndex();
        std::vector<const PatientRecord*> found;
        QString used;

        if (field == 0) {                                          // ----- by ID -----
            bool ok = false;
            const int id = raw.toInt(&ok);
            if (!ok)
                throw InvalidSearchException("Patient ID must be a whole number.");

            int index = -1;
            if (algo == 0) {
                index = SearchEngine::findById(db, id);
                used = db.sortedField() == SortField::Id
                           ? "Binary Search (the list is sorted by ID)"
                           : "Linear Search (the list is not sorted by ID)";
            } else if (algo == 1) {
                index = SearchEngine::linearSearchById(db.patients(), id);
                used = "Linear Search";
            } else {
                if (db.sortedField() != SortField::Id)
                    throw InvalidSearchException(
                        "Binary search only works on a list sorted by ID. Sort by Patient ID first.");
                index = SearchEngine::binarySearchById(db.patients(), id);
                used = "Binary Search";
            }
            if (index >= 0) {
                PatientRecord* p = db.patients()[static_cast<size_t>(index)].get();
                db.touch(p);                                       // feeds the recent-patients cache
                found.push_back(p);
            }
        } else if (field == 1) {                                   // ----- by name -----
            for (int index : SearchEngine::searchByName(db, raw.toStdString())) {
                PatientRecord* p = db.patients()[static_cast<size_t>(index)].get();
                db.touch(p);
                found.push_back(p);
            }
            used = "Recursive Search";
        } else {                                                   // ----- by category -----
            const std::string category = canonicalCategory(raw.toStdString());
            for (int index : SearchEngine::searchByCategory(db.patients(), category))
                found.push_back(db.patients()[static_cast<size_t>(index)].get());
            used = "Linear Scan";
        }

        showPatients(found);
        resultLabel_->setText(found.empty()
            ? QString("No patients found (searched with %1).").arg(used)
            : QString("Found %1 patient(s) using %2.").arg(static_cast<int>(found.size())).arg(used));
    } catch (const ClinicException& e) {
        QMessageBox::warning(this, "Invalid search",
                             QString("Error: ") + QString::fromStdString(e.what()));
    } catch (...) {
        QMessageBox::critical(this, "Unexpected error", "An unknown error occurred.");
    }
}

void SearchSortPanel::onShowAll()
{
    searchEdit_->clear();
    refresh();
}

void SearchSortPanel::onSort()
{
    try {
        const SortAlgorithm algorithm = algorithmFromIndex(sortAlgoCombo_->currentIndex());
        const SortField field = fieldFromIndex(sortFieldCombo_->currentIndex());
        SortEngine::sort(clinic_.database(), algorithm, field);

        const QString message = QString("Sorted %1 patient(s) by %2 using %3.")
                                    .arg(static_cast<int>(clinic_.database().size()))
                                    .arg(sortFieldCombo_->currentText())
                                    .arg(QString::fromStdString(SortEngine::algorithmName(algorithm)));
        emit orderChanged();               // every panel redraws in the new order
        resultLabel_->setText(message);
    } catch (const ClinicException& e) {
        QMessageBox::warning(this, "Could not sort",
                             QString("Error: ") + QString::fromStdString(e.what()));
    } catch (...) {
        QMessageBox::critical(this, "Unexpected error", "An unknown error occurred.");
    }
}

} // namespace carepilot::gui