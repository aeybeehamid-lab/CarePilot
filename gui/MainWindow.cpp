#include "gui/MainWindow.h"
#include "gui/PatientPanel.h"
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

namespace scapms::gui {

static QWidget* placeholderTab(const QString& text)
{
    auto* label = new QLabel(text);
    label->setAlignment(Qt::AlignCenter);
    return label;
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    patientPanel_ = new PatientPanel(clinic_, this);

    tabs_ = new QTabWidget(this);
    tabs_->addTab(patientPanel_, "Patient Records");
    tabs_->addTab(placeholderTab("Appointments - coming in the next step"), "Appointments");
    tabs_->addTab(placeholderTab("Waiting Queue - coming in the next step"), "Waiting Queue");
    tabs_->addTab(placeholderTab("Search & Sort - coming in the next step"), "Search && Sort");
    tabs_->addTab(placeholderTab("Reports - coming in the next step"), "Reports");
    setCentralWidget(tabs_);

    statsLabel_ = new QLabel(this);
    statusBar()->addPermanentWidget(statsLabel_);

    createMenus();

    connect(patientPanel_, &PatientPanel::dataChanged, this, &MainWindow::markDirtyAndRefresh);
    connect(patientPanel_, &PatientPanel::undoRequested, this, &MainWindow::onUndo);

    // Start with the sample data if it is available.
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
    addMenuAction(editMenu, "&Find Patient", "Ctrl+F", "Jump to the patient filter box", &MainWindow::onFind);

    QMenu* helpMenu = menuBar()->addMenu("&Help");
    addMenuAction(helpMenu, "&About", "F1", "About SCAPMS", &MainWindow::onAbout);
}

void MainWindow::refreshAll()
{
    patientPanel_->refresh();

    const bool canUndo = clinic_.canUndo();
    undoAction_->setEnabled(canUndo);
    undoAction_->setText(canUndo
        ? "&Undo: " + QString::fromStdString(clinic_.undoDescription())
        : QString("&Undo Last Operation"));

    statsLabel_->setText(QString("Patients: %1   |   Waiting in queue: %2")
                             .arg(static_cast<int>(clinic_.database().size()))
                             .arg(static_cast<int>(clinic_.queueSize())));
    updateTitle();
}

void MainWindow::updateTitle()
{
    const QString name = currentFile_.isEmpty() ? QString("Untitled")
                                                : QFileInfo(currentFile_).fileName();
    setWindowTitle(name + (dirty_ ? "*" : "") +
                   " - SCAPMS | Smart Clinic Appointment & Patient Management System");
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
                                                      "SCAPMS data (*.txt);;All files (*)");
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
                                                      "SCAPMS data (*.txt);;All files (*)");
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
    tabs_->setCurrentWidget(patientPanel_);
    patientPanel_->focusFilter();
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, "About SCAPMS",
        "<b>SCAPMS</b><br>Smart Clinic Appointment &amp; Patient Management System<br><br>"
        "COS202 Programming II - Lab Assessment Project");
}

} // namespace scapms::gui