#include "gui/MainWindow.h"
#include "gui/AppointmentPanel.h"
#include "gui/GuiHelpers.h"
#include "gui/PatientPanel.h"
#include "gui/QueuePanel.h"
#include "gui/ReportPanel.h"
#include "gui/SearchSortPanel.h"
#include "utils/ClinicException.h"
#include "utils/FileHandler.h"
#include <QAction>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QTabWidget>
#include <QTimer>

namespace carepilot::gui {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    patientPanel_     = new PatientPanel(clinic_, this);
    appointmentPanel_ = new AppointmentPanel(clinic_, this);
    queuePanel_       = new QueuePanel(clinic_, this);
    searchSortPanel_  = new SearchSortPanel(clinic_, this);
    reportPanel_      = new ReportPanel(clinic_, this);

    tabs_ = new QTabWidget(this);
    tabs_->addTab(patientPanel_, "Patient Records");
    tabs_->addTab(appointmentPanel_, "Appointments");
    tabs_->addTab(queuePanel_, "Waiting Queue");
    tabs_->addTab(searchSortPanel_, "Search && Sort");
    tabs_->addTab(reportPanel_, "Reports");
    setCentralWidget(tabs_);

    statsLabel_ = new QLabel(this);
    statusBar()->addPermanentWidget(statsLabel_);

    createMenus();

    connect(patientPanel_, &PatientPanel::dataChanged, this, &MainWindow::markDirtyAndRefresh);
    connect(patientPanel_, &PatientPanel::undoRequested, this, &MainWindow::onUndo);
    connect(appointmentPanel_, &AppointmentPanel::dataChanged, this, &MainWindow::markDirtyAndRefresh);
    connect(queuePanel_, &QueuePanel::queueChanged, this, &MainWindow::refreshAll);
    connect(searchSortPanel_, &SearchSortPanel::orderChanged, this, &MainWindow::refreshAll);

    // Timer event: once per second refresh waiting times and raise the long-wait alert.
    timer_ = new QTimer(this);
    timer_->setInterval(1000);
    connect(timer_, &QTimer::timeout, this, &MainWindow::onTick);
    timer_->start();

    const QString sample = QCoreApplication::applicationDirPath() + "/data/sample_data.txt";
    if (QFileInfo::exists(sample)) {
        try {
            FileHandler::load(clinic_, sample.toStdString());
            statusBar()->showMessage("Sample data loaded. Use File > Save As to keep your own copy.", 6000);
        } catch (const ClinicException& e) {
            statusBar()->showMessage(QString("Could not load sample data: ") + e.what(), 8000);
        }
    }
    refreshAll();
}

QAction* MainWindow::addMenuAction(QMenu* menu, const QString& text, const QString& shortcut,
                                   const QString& tip, void (MainWindow::*slot)())
{
    QAction* action = new QAction(text, this);
    if (!shortcut.isEmpty()) action->setShortcut(QKeySequence(shortcut));
    action->setStatusTip(tip);
    action->setToolTip(tip);
    connect(action, &QAction::triggered, this, slot);
    menu->addAction(action);
    return action;
}

void MainWindow::createMenus()
{
    QMenu* fileMenu = menuBar()->addMenu("&File");
    addMenuAction(fileMenu, "&New", "Ctrl+N", "Start with an empty clinic database", &MainWindow::onNew);
    addMenuAction(fileMenu, "&Load...", "Ctrl+O", "Load patients and appointments from a file", &MainWindow::onLoad);
    addMenuAction(fileMenu, "&Save", "Ctrl+S", "Save patients and appointments", &MainWindow::onSave);
    addMenuAction(fileMenu, "Save &As...", "Ctrl+Shift+S", "Save to a new file", &MainWindow::onSaveAs);
    fileMenu->addSeparator();
    addMenuAction(fileMenu, "E&xit", "Ctrl+Q", "Close the application", &MainWindow::onExit);

    QMenu* editMenu = menuBar()->addMenu("&Edit");
    undoAction_ = addMenuAction(editMenu, "&Undo Last Operation", "Ctrl+Z",
                                "Reverse the last delete, edit or appointment cancellation", &MainWindow::onUndo);
    addMenuAction(editMenu, "&Find Patient", "Ctrl+F", "Go to the Search & Sort tab", &MainWindow::onFind);

    QMenu* helpMenu = menuBar()->addMenu("&Help");
    addMenuAction(helpMenu, "&About", "F1", "About CarePilot", &MainWindow::onAbout);
}

void MainWindow::refreshAll()
{
    patientPanel_->refresh();
    appointmentPanel_->refresh();
    queuePanel_->refresh();
    searchSortPanel_->refresh();
    reportPanel_->refresh();

    const bool canUndo = clinic_.canUndo();
    undoAction_->setEnabled(canUndo);
    undoAction_->setText(canUndo
        ? "&Undo: " + QString::fromStdString(clinic_.undoDescription())
        : QString("&Undo Last Operation"));

    updateStats();
    updateTitle();
}

void MainWindow::updateStats()
{
    const size_t waiting = clinic_.queueSize();
    const long longest = clinic_.longestWaitSeconds();

    QString text = QString("Patients: %1   |   Waiting in queue: %2")
                       .arg(static_cast<int>(clinic_.database().size()))
                       .arg(static_cast<int>(waiting));
    if (waiting > 0)
        text += "   |   Longest wait: " + formatWait(longest);

    const bool alert = waiting > 0 && longest >= WAIT_ALERT_SECONDS;
    if (alert) text += "   ALERT: patient waiting too long!";
    statsLabel_->setText(text);
    statsLabel_->setStyleSheet(alert ? "color:#c62828; font-weight:bold;" : "");
}

void MainWindow::onTick()
{
    queuePanel_->refreshTable();
    updateStats();
}

void MainWindow::updateTitle()
{
    const QString name = currentFile_.isEmpty() ? QString("Untitled")
                                                : QFileInfo(currentFile_).fileName();
    setWindowTitle(name + (dirty_ ? "*" : "") +
                   " - CarePilot | Smart Clinic Appointment & Patient Management System");
}

void MainWindow::markDirtyAndRefresh()
{
    dirty_ = true;
    refreshAll();
}

bool MainWindow::confirmDiscardChanges()
{
    if (!dirty_) return true;
    const auto answer = QMessageBox::question(this, "Unsaved changes",
        "You have unsaved changes. Save them first?",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
    if (answer == QMessageBox::Save) return save();
    return answer == QMessageBox::Discard;
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (confirmDiscardChanges()) event->accept();
    else event->ignore();
}

// ---------------- File menu ----------------

void MainWindow::onNew()
{
    if (!confirmDiscardChanges()) return;
    clinic_ = ClinicManager();
    currentFile_.clear();
    dirty_ = false;
    refreshAll();
    statusBar()->showMessage("New empty database created.", 4000);
}

void MainWindow::onLoad()
{
    if (!confirmDiscardChanges()) return;
    const QString path = QFileDialog::getOpenFileName(this, "Load clinic data", QString(),
                                                      "CarePilot data (*.txt);;All files (*)");
    if (path.isEmpty()) return;

    try {
        FileHandler::load(clinic_, path.toStdString());
        currentFile_ = path;
        dirty_ = false;
        refreshAll();
        statusBar()->showMessage("Loaded " + path, 5000);
    } catch (const ClinicException& e) {
        QMessageBox::critical(this, "Load failed", QString::fromStdString(e.what()));
    } catch (...) {
        QMessageBox::critical(this, "Load failed", "An unexpected error occurred while loading.");
    }
}

bool MainWindow::saveTo(const QString& path)
{
    try {
        FileHandler::save(clinic_, path.toStdString());
    } catch (const ClinicException& e) {
        QMessageBox::critical(this, "Save failed", QString::fromStdString(e.what()));
        return false;
    } catch (...) {
        QMessageBox::critical(this, "Save failed", "An unexpected error occurred while saving.");
        return false;
    }
    currentFile_ = path;
    dirty_ = false;
    updateTitle();
    statusBar()->showMessage("Saved to " + path, 5000);
    return true;
}

bool MainWindow::save()
{
    if (currentFile_.isEmpty()) return saveAs();
    return saveTo(currentFile_);
}

bool MainWindow::saveAs()
{
    const QString path = QFileDialog::getSaveFileName(this, "Save clinic data", "clinic_data.txt",
                                                      "CarePilot data (*.txt);;All files (*)");
    if (path.isEmpty()) return false;
    return saveTo(path);
}

void MainWindow::onSave()   { save(); }
void MainWindow::onSaveAs() { saveAs(); }
void MainWindow::onExit()   { close(); }

// ---------------- Edit / Help ----------------

void MainWindow::onUndo()
{
    try {
        const QString message = QString::fromStdString(clinic_.undoLastOperation());
        markDirtyAndRefresh();
        statusBar()->showMessage(message, 5000);
    } catch (const ClinicException& e) {
        QMessageBox::information(this, "Undo", QString::fromStdString(e.what()));
    }
}

void MainWindow::onFind()
{
    tabs_->setCurrentWidget(searchSortPanel_);
    searchSortPanel_->focusSearch();
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, "About CarePilot",
        "<b>CarePilot</b><br>Smart Clinic Appointment &amp; Patient Management System<br><br>"
        "COS202 Programming II - Lab Assessment Project");
}

} // namespace carepilot::gui