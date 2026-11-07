#pragma once
#include "controller/ClinicManager.h"
#include <QWidget>
#include <string>

class QLineEdit;
class QPlainTextEdit;

namespace carepilot::gui {

class ReportPanel : public QWidget {
    Q_OBJECT
public:
    explicit ReportPanel(ClinicManager& clinic, QWidget* parent = nullptr);
    void refresh();      // re-runs the report on screen so it never shows stale data

private slots:
    void onExport();

private:
    void showReport(int which);          // 0..3 = single report, 4 = all reports
    std::string build(int which);

    ClinicManager& clinic_;
    QLineEdit* dateEdit_;
    QPlainTextEdit* output_;
    int current_ = -1;
};

} // namespace carepilot::gui