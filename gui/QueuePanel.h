#pragma once
#include "controller/ClinicManager.h"
#include <QWidget>

class QComboBox;
class QLabel;
class QPushButton;
class QTableWidget;

namespace carepilot::gui {

class QueuePanel : public QWidget {
    Q_OBJECT
public:
    explicit QueuePanel(ClinicManager& clinic, QWidget* parent = nullptr);
    void refresh();          // rebuilds the patient dropdown and the table
    void refreshTable();     // table and waiting times only (called by the timer)

signals:
    void queueChanged();

private slots:
    void onAdd();
    void onCallNext();

private:
    ClinicManager& clinic_;
    QComboBox* patientCombo_;
    QPushButton* addButton_;
    QPushButton* callNextButton_;
    QLabel* nowServing_;
    QLabel* summary_;
    QTableWidget* table_;
};

} // namespace carepilot::gui