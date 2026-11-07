#include "gui/DashboardPanel.h"
#include "controller/ReportGenerator.h"
#include "gui/GuiHelpers.h"
#include <QDate>
#include <QDateTime>
#include <QFont>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QProgressBar>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>
#include <string>
#include <vector>

namespace carepilot::gui {

// An appointment starting within this many minutes triggers a reminder.
static constexpr int REMINDER_MINUTES = 30;

struct UpcomingRow {
    int appointmentId;
    std::string patientName;
    std::string date;
    std::string time;
    std::string reason;
    QDateTime when;
};

static QProgressBar* makeBar(QWidget* parent, const QString& color)
{
    auto* bar = new QProgressBar(parent);
    bar->setFormat("%v");
    bar->setAlignment(Qt::AlignCenter);
    bar->setStyleSheet(QString(
        "QProgressBar { border: 1px solid gray; border-radius: 4px; text-align: center; height: 22px; }"
        "QProgressBar::chunk { background: %1; border-radius: 3px; }").arg(color));
    return bar;
}

QFrame* DashboardPanel::makeCard(const QString& title, const QString& accent,
                                 QLabel*& value, QLabel*& detail)
{
    auto* card = new QFrame(this);
    card->setObjectName("card");
    card->setStyleSheet(QString(
        "QFrame#card { background: palette(base); border: 1px solid palette(mid);"
        " border-left: 6px solid %1; border-radius: 8px; }").arg(accent));

    auto* titleLabel = new QLabel(title, card);
    titleLabel->setStyleSheet("font-weight: bold;");

    value = new QLabel("0", card);
    QFont big = value->font();
    big.setPointSize(28);
    big.setBold(true);
    value->setFont(big);

    detail = new QLabel(card);
    detail->setStyleSheet("color: gray;");

    auto* layout = new QVBoxLayout(card);
    layout->addWidget(titleLabel);
    layout->addWidget(value);
    layout->addWidget(detail);
    return card;
}

DashboardPanel::DashboardPanel(ClinicManager& clinic, QWidget* parent)
    : QWidget(parent), clinic_(clinic)
{
    headerLabel_ = new QLabel(this);
    QFont headerFont = headerLabel_->font();
    headerFont.setPointSize(headerFont.pointSize() + 4);
    headerFont.setBold(true);
    headerLabel_->setFont(headerFont);

    // ---- Row 1: statistic cards ----
    auto* cards = new QHBoxLayout;
    cards->addWidget(makeCard("Total patients", "#2e7d32", patientsValue_, patientsDetail_), 1);
    cards->addWidget(makeCard("Pending appointments", "#f9a825", pendingValue_, pendingDetail_), 1);
    cards->addWidget(makeCard("Today's appointments", "#1565c0", todayValue_, todayDetail_), 1);
    cards->addWidget(makeCard("Waiting queue", "#c62828", queueValue_, queueDetail_), 1);

    // ---- Row 2, panel 1: category bars ----
    emergencyBar_ = makeBar(this, "#e05555");
    followUpBar_  = makeBar(this, "#4a86e8");
    generalBar_   = makeBar(this, "#4caf50");
    auto* categoryBox = new QGroupBox("Patient categories", this);
    auto* categoryForm = new QFormLayout(categoryBox);
    categoryForm->addRow("Emergency", emergencyBar_);
    categoryForm->addRow("Follow-Up", followUpBar_);
    categoryForm->addRow("General", generalBar_);

    // ---- Row 2, panel 2: recently accessed patients (the array cache) ----
    recentList_ = new QListWidget(this);
    recentList_->setToolTip("The last patients you opened, searched for or booked. Most recent first.");
    auto* recentBox = new QGroupBox("Recently accessed patients", this);
    auto* recentLayout = new QVBoxLayout(recentBox);
    recentLayout->addWidget(recentList_);

    // ---- Row 2, panel 3: upcoming appointments and reminders ----
    upcomingList_ = new QListWidget(this);
    upcomingList_->setToolTip("Pending appointments, earliest first. Red = overdue, amber = starting soon.");
    auto* upcomingBox = new QGroupBox("Upcoming appointments & reminders", this);
    auto* upcomingLayout = new QVBoxLayout(upcomingBox);
    upcomingLayout->addWidget(upcomingList_);

    auto* panels = new QHBoxLayout;
    panels->addWidget(categoryBox, 2);
    panels->addWidget(recentBox, 2);
    panels->addWidget(upcomingBox, 3);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(headerLabel_);
    layout->addLayout(cards);
    layout->addLayout(panels, 1);

    // Timer event: refresh the dashboard (and check reminders) every 30 seconds.
    timer_ = new QTimer(this);
    timer_->setInterval(30000);
    connect(timer_, &QTimer::timeout, this, [this] { refresh(); });
    timer_->start();

    refresh();
}

void DashboardPanel::refresh()
{
    headerLabel_->setText("Clinic overview - " + QDate::currentDate().toString("dddd, d MMMM yyyy"));
    refreshStatistics();
    refreshRecent();
    refreshUpcoming();
}

void DashboardPanel::refreshStatistics()
{
    const ClinicDatabase& db = clinic_.database();
    const std::string today = ReportGenerator::todayDate();

    int general = 0, emergency = 0, followUp = 0;
    int pending = 0, patientsWithPending = 0;
    int todayTotal = 0, todayDone = 0, todayPending = 0;

    for (const auto& p : db.patients()) {
        const std::string category = p->category();
        if (category == "General")        ++general;
        else if (category == "Emergency") ++emergency;
        else                              ++followUp;

        const int patientPending = p->countByStatus(AppointmentStatus::Pending);
        pending += patientPending;
        if (patientPending > 0) ++patientsWithPending;

        for (const Appointment& a : p->appointments()) {
            if (a.date() != today) continue;
            ++todayTotal;
            if (a.status() == AppointmentStatus::Completed) ++todayDone;
            if (a.status() == AppointmentStatus::Pending)   ++todayPending;
        }
    }

    const int total = static_cast<int>(db.size());
    patientsValue_->setText(QString::number(total));
    patientsDetail_->setText(QString("%1 emergency case(s)").arg(emergency));
    pendingValue_->setText(QString::number(pending));
    pendingDetail_->setText(QString("across %1 patient(s)").arg(patientsWithPending));
    todayValue_->setText(QString::number(todayTotal));
    todayDetail_->setText(QString("%1 done, %2 pending").arg(todayDone).arg(todayPending));
    refreshQueueCard();

    const int maxValue = std::max(total, 1);
    generalBar_->setRange(0, maxValue);
    emergencyBar_->setRange(0, maxValue);
    followUpBar_->setRange(0, maxValue);
    generalBar_->setValue(general);
    emergencyBar_->setValue(emergency);
    followUpBar_->setValue(followUp);
}

void DashboardPanel::refreshQueueCard()
{
    const size_t waiting = clinic_.queueSize();
    queueValue_->setText(QString::number(static_cast<int>(waiting)));

    if (waiting == 0) {
        queueDetail_->setText("Nobody waiting");
        queueDetail_->setStyleSheet("color: gray;");
        return;
    }
    const long longest = clinic_.longestWaitSeconds();
    queueDetail_->setText("Longest wait " + formatWait(longest));
    queueDetail_->setStyleSheet(longest >= WAIT_ALERT_SECONDS ? "color: #c62828; font-weight: bold;"
                                                              : "color: gray;");
}

void DashboardPanel::refreshRecent()
{
    recentList_->clear();
    const ClinicDatabase& db = clinic_.database();

    if (db.cacheCount() == 0) {
        auto* item = new QListWidgetItem("Nobody yet. Search for, edit or book a patient to fill this list.");
        item->setFlags(Qt::NoItemFlags);
        recentList_->addItem(item);
        return;
    }

    for (int slot = db.cacheCount() - 1; slot >= 0; --slot) {   // newest first
        const PatientRecord* p = db.cachedAt(slot);
        if (!p) continue;
        auto* item = new QListWidgetItem(QString("%1 - %2   [%3]")
                                             .arg(p->id())
                                             .arg(QString::fromStdString(p->name()))
                                             .arg(QString::fromStdString(p->category())));
        item->setBackground(priorityColor(p->priority()));
        item->setForeground(QColor("#1b1b1b"));
        recentList_->addItem(item);
    }
}

void DashboardPanel::refreshUpcoming()
{
    // Collect every pending appointment.
    std::vector<UpcomingRow> rows;
    for (const auto& p : clinic_.database().patients()) {
        for (const Appointment& a : p->appointments()) {
            if (a.status() != AppointmentStatus::Pending) continue;
            rows.push_back({a.id(), p->name(), a.date(), a.time(), a.reason(),
                            QDateTime::fromString(QString::fromStdString(a.date() + " " + a.time()),
                                                  "yyyy-MM-dd HH:mm")});
        }
    }

    // Insertion sort, earliest first.
    for (size_t i = 1; i < rows.size(); ++i) {
        UpcomingRow key = rows[i];
        size_t j = i;
        while (j > 0 && key.when < rows[j - 1].when) {
            rows[j] = rows[j - 1];
            --j;
        }
        rows[j] = key;
    }

    upcomingList_->clear();
    if (rows.empty()) {
        auto* item = new QListWidgetItem("No pending appointments.");
        item->setFlags(Qt::NoItemFlags);
        upcomingList_->addItem(item);
        return;
    }

    const QDateTime now = QDateTime::currentDateTime();
    const QString todayText = QDate::currentDate().toString("yyyy-MM-dd");
    const size_t shown = std::min<size_t>(rows.size(), 12);

    for (size_t i = 0; i < shown; ++i) {
        const UpcomingRow& r = rows[i];
        const qint64 minutes = now.secsTo(r.when) / 60;
        const QString date = QString::fromStdString(r.date);
        const QString when = (date == todayText ? QString("Today") : date) + " " +
                             QString::fromStdString(r.time);
        QString text = QString("%1   %2 - %3")
                           .arg(when)
                           .arg(QString::fromStdString(r.patientName))
                           .arg(QString::fromStdString(r.reason));

        auto* item = new QListWidgetItem();
        item->setForeground(QColor("#1b1b1b"));
        if (minutes < 0) {
            text = "OVERDUE   " + text;
            item->setBackground(QColor("#ffcdd2"));
        } else if (minutes <= REMINDER_MINUTES) {
            text = QString("In %1 min   ").arg(minutes) + text;
            item->setBackground(QColor("#ffe082"));
        }
        item->setText(text);
        upcomingList_->addItem(item);
    }

    // Reminder events: announce each appointment starting soon exactly once.
    for (const UpcomingRow& r : rows) {
        const qint64 minutes = now.secsTo(r.when) / 60;
        if (minutes < 0 || minutes > REMINDER_MINUTES) continue;
        if (reminded_.contains(r.appointmentId)) continue;
        reminded_.insert(r.appointmentId);
        emit appointmentReminder(QString("Reminder: %1 has an appointment at %2 (in %3 min) - %4")
                                     .arg(QString::fromStdString(r.patientName))
                                     .arg(QString::fromStdString(r.time))
                                     .arg(minutes)
                                     .arg(QString::fromStdString(r.reason)));
    }
}

} // namespace carepilot::gui