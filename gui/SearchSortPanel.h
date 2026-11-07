#pragma once
#include "controller/ClinicManager.h"
#include <QWidget>
#include <vector>

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;

namespace carepilot::gui {

class SearchSortPanel : public QWidget {
    Q_OBJECT
public:
    explicit SearchSortPanel(ClinicManager& clinic, QWidget* parent = nullptr);
    void refresh();          // show the whole list again
    void focusSearch();

signals:
    void orderChanged();

private slots:
    void onSearchFieldChanged(int index);
    void onSortAlgorithmChanged(int index);
    void onSearch();
    void onShowAll();
    void onSort();

private:
    void showPatients(const std::vector<const PatientRecord*>& patients);
    void updateSortedLabel();

    ClinicManager& clinic_;
    QComboBox* fieldCombo_;
    QComboBox* algoCombo_;
    QLineEdit* searchEdit_;
    QPushButton* searchButton_;
    QPushButton* showAllButton_;
    QComboBox* sortAlgoCombo_;
    QComboBox* sortFieldCombo_;
    QPushButton* sortButton_;
    QLabel* explanationLabel_;
    QLabel* sortedLabel_;
    QLabel* resultLabel_;
    QTableWidget* table_;
};

} // namespace carepilot::gui