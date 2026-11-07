#pragma once
#include <QChar>
#include <QColor>
#include <QString>

namespace carepilot::gui {

// Waiting longer than this (seconds) raises the alert in the status bar.
// Set it to 10 while testing, then back to 300.
constexpr long WAIT_ALERT_SECONDS = 300;

inline QColor priorityColor(int priority)
{
    switch (priority) {
        case 1:  return QColor("#ffd9d9");   // Emergency
        case 2:  return QColor("#d9e8ff");   // Follow-Up
        default: return QColor("#e4f5e4");   // General
    }
}

inline QString formatWait(long seconds)
{
    if (seconds < 0) seconds = 0;
    return QString("%1:%2")
        .arg(seconds / 60, 2, 10, QChar('0'))
        .arg(seconds % 60, 2, 10, QChar('0'));
}

} // namespace carepilot::gui