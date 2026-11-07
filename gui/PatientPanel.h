#pragma once
#include "controller/ClinicManager.h"
#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;

namespace carepilot::gui {

class PatientPanel : public QWidget {
    Q_OBJECT
public:
    explicit PatientPanel(ClinicManager& clinic, QWidget* parent = nullptr);

    void refresh();                 // rebuild the table from the database
    void focusFilter();
    int  selectedPatientId() const; // -1 when nothing is selected

signals:
    void dataChanged();
    void undoRequested();

private slots:
    void onAdd();
    void onEdit();
    void onDelete();
    void onFilterChanged(const QString& text);
    void onSelectionChanged();

private:
    ClinicManager& clinic_;
    QLineEdit* filterEdit_;
    QTableWidget* table_;
    QPushButton* addButton_;
    QPushButton* editButton_;
    QPushButton* deleteButton_;
    QPushButton* undoButton_;
    QLabel* countLabel_;
};

} // namespace carepilot::gui