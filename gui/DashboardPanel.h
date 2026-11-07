#pragma once
#include "controller/ClinicManager.h"
#include <QSet>
#include <QWidget>

class QFrame;
class QLabel;
class QListWidget;
class QProgressBar;
class QTimer;

namespace carepilot::gui {

class DashboardPanel : public QWidget {
    Q_OBJECT
public:
    explicit DashboardPanel(ClinicManager& clinic, QWidget* parent = nullptr);
    void refresh();            // rebuild everything
    void refreshQueueCard();   // cheap update, called every second by MainWindow's timer

signals:
    void appointmentReminder(const QString& text);

private:
    QFrame* makeCard(const QString& title, const QString& accent, QLabel*& value, QLabel*& detail);
    void refreshStatistics();
    void refreshRecent();
    void refreshUpcoming();

    ClinicManager& clinic_;
    QLabel* headerLabel_;
    QLabel* patientsValue_;
    QLabel* patientsDetail_;
    QLabel* pendingValue_;
    QLabel* pendingDetail_;
    QLabel* todayValue_;
    QLabel* todayDetail_;
    QLabel* queueValue_;
    QLabel* queueDetail_;
    QProgressBar* generalBar_;
    QProgressBar* emergencyBar_;
    QProgressBar* followUpBar_;
    QListWidget* recentList_;
    QListWidget* upcomingList_;
    QSet<int> reminded_;       // appointment IDs already announced, so each reminder fires once
    QTimer* timer_;
};

} // namespace carepilot::gui