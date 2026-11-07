#pragma once
#include "controller/ClinicManager.h"
#include <QMainWindow>
#include <QString>

class QAction;
class QLabel;
class QMenu;
class QTabWidget;

namespace scapms::gui {

class PatientPanel;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onNew();
    void onLoad();
    void onSave();
    void onSaveAs();
    void onExit();
    void onUndo();
    void onFind();
    void onAbout();
    void markDirtyAndRefresh();

private:
    void createMenus();
    QAction* addMenuAction(QMenu* menu, const QString& text, const QString& shortcut,
                           const QString& tip, void (MainWindow::*slot)());
    void refreshAll();
    void updateTitle();
    bool save();
    bool saveAs();
    bool saveTo(const QString& path);
    bool confirmDiscardChanges();

    ClinicManager clinic_;
    QTabWidget* tabs_ = nullptr;
    PatientPanel* patientPanel_ = nullptr;
    QLabel* statsLabel_ = nullptr;
    QAction* undoAction_ = nullptr;
    QString currentFile_;
    bool dirty_ = false;
};

} // namespace scapms::gui