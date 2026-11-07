#pragma once
#include "controller/ClinicManager.h"
#include <QWidget>

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;

namespace carepilot::gui {

class AppointmentPanel : public QWidget {
    Q_OBJECT
public:
    explicit AppointmentPanel(ClinicManager& clinic, QWidget* parent = nullptr);
    void refresh();

signals:
    void dataChanged();

private slots:
    void onSchedule();
    void onCancel();
    void onComplete();
    void onSelectionChanged();

private:
    void refreshPatientCombo();
    void refreshTable();
    int selectedAppointmentId() const;   // -1 when nothing is selected
    int selectedPatientId() const;

    ClinicManager& clinic_;
    QComboBox* patientCombo_;
    QLineEdit* dateEdit_;
    QLineEdit* timeEdit_;
    QLineEdit* reasonEdit_;
    QPushButton* scheduleButton_;
    QPushButton* cancelButton_;
    QPushButton* completeButton_;
    QComboBox* statusFilter_;
    QTableWidget* table_;
    QLabel* countLabel_;
};

} // namespace carepilot::gui