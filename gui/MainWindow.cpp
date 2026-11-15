#include "gui/MainWindow.h"

#include <QAction>
#include <QFileDialog>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QTabWidget>

#include "gui/AppointmentPanel.h"
#include "gui/DashboardPanel.h"
#include "gui/PatientPanel.h"
#include "gui/QueuePanel.h"
#include "gui/ReportPanel.h"
#include "gui/SearchSortPanel.h"
#include "utils/ClinicException.h"

namespace scapms {
namespace gui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), tabs(nullptr), summaryLabel(nullptr),
      undoAction(nullptr), searchPanel(nullptr) {

    setWindowTitle("SCAPMS - Smart Clinic Appointment & Patient Management System");
    resize(1100, 700);

    tabs = new QTabWidget(this);
    setCentralWidget(tabs);

    createMenus();
    createStatusBar();

    // Each tab is one line here
    addPanel(new DashboardPanel(clinic), "Dashboard");
    addPanel(new PatientPanel(clinic), "Patient Records");
    addPanel(new AppointmentPanel(clinic), "Appointments");
    addPanel(new QueuePanel(clinic), "Waiting Queue");

    searchPanel = new SearchSortPanel(clinic);
    addPanel(searchPanel, "Search && Sort");        // && shows a single &

    addPanel(new ReportPanel(clinic), "Reports");

    refreshAll();
    showStatus("Ready");
}

// ============================ set-up ============================

void MainWindow::createMenus() {
    // ---- File menu ----
    QMenu* fileMenu = menuBar()->addMenu("&File");

    QAction* newAction = fileMenu->addAction("&New");
    newAction->setShortcut(QKeySequence::New);                       // Ctrl+N
    newAction->setStatusTip("Remove all patients and start with an empty clinic");
    connect(newAction, &QAction::triggered, this, &MainWindow::newDatabase);

    QAction* loadAction = fileMenu->addAction("&Load...");
    loadAction->setShortcut(QKeySequence::Open);                     // Ctrl+O
    loadAction->setStatusTip("Load patients and appointments from a file");
    connect(loadAction, &QAction::triggered, this, &MainWindow::loadFile);

    QAction* saveAction = fileMenu->addAction("&Save");
    saveAction->setShortcut(QKeySequence::Save);                     // Ctrl+S
    saveAction->setStatusTip("Save patients and appointments to a file");
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveFile);

    QAction* saveAsAction = fileMenu->addAction("Save &As...");
    saveAsAction->setShortcut(QKeySequence::SaveAs);                 // Ctrl+Shift+S
    saveAsAction->setStatusTip("Save to a new file");
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::saveFileAs);

    fileMenu->addSeparator();

    QAction* exitAction = fileMenu->addAction("E&xit");
    exitAction->setShortcut(QKeySequence("Ctrl+Q"));
    exitAction->setStatusTip("Close the application");
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    // ---- Edit menu ----
    QMenu* editMenu = menuBar()->addMenu("&Edit");

    undoAction = editMenu->addAction("&Undo last operation");
    undoAction->setShortcut(QKeySequence::Undo);                     // Ctrl+Z
    undoAction->setStatusTip("Reverse the most recent administrative operation");
    connect(undoAction, &QAction::triggered, this, &MainWindow::undoLast);

    editMenu->addSeparator();

    QAction* findAction = editMenu->addAction("&Find patient...");
    findAction->setShortcut(QKeySequence::Find);                     // Ctrl+F
    findAction->setStatusTip("Jump to the Search & Sort tab and start typing");
    connect(findAction, &QAction::triggered, this, &MainWindow::focusSearch);

    // ---- Help menu ----
    QMenu* helpMenu = menuBar()->addMenu("&Help");

    QAction* aboutAction = helpMenu->addAction("&About");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAbout);
}

void MainWindow::createStatusBar() {
    summaryLabel = new QLabel();
    statusBar()->addPermanentWidget(summaryLabel);    // stays on the right
}

void MainWindow::addPanel(ClinicPanel* panel, const QString& title) {
    tabs->addTab(panel, title);
    panels.push_back(panel);

    // Event wiring: the panel announces, the window reacts
    connect(panel, &ClinicPanel::dataChanged, this, &MainWindow::refreshAll);
    connect(panel, &ClinicPanel::statusMessage, this, &MainWindow::showStatus);
}

// ============================ slots ============================

void MainWindow::refreshAll() {
    // Polymorphism: every tab is refreshed through the ClinicPanel base pointer
    for (size_t i = 0; i < panels.size(); i++) {
        panels[i]->refresh();
    }

    controller::DashboardStats stats = clinic.getStats();
    summaryLabel->setText(QString("Patients: %1   |   Waiting: %2   |   Undo available: %3")
                              .arg(stats.totalPatients)
                              .arg(stats.waitingCount)
                              .arg(stats.undoDepth));

    undoAction->setEnabled(clinic.canUndo());     // greyed out when there is nothing to undo
}

void MainWindow::showStatus(const QString& text) {
    statusBar()->showMessage(text, 6000);         // disappears after 6 seconds
}

void MainWindow::newDatabase() {
    QMessageBox::StandardButton answer = QMessageBox::question(
        this, "New clinic",
        "Remove all patients and start with an empty clinic?\nUnsaved data will be lost.",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) return;

    clinic.newDatabase();
    currentFile.clear();
    refreshAll();
    showStatus("Started a new, empty clinic.");
}

void MainWindow::saveTo(const QString& path) {
    try {
        clinic.saveToFile(path.toStdString());
        currentFile = path;
        showStatus("Saved to " + path);
    } catch (const utils::ClinicException& e) {
        QMessageBox::warning(this, "Save failed", QString::fromUtf8(e.what()));
    }
}

void MainWindow::saveFile() {
    if (currentFile.isEmpty()) {
        saveFileAs();                // no file yet: ask where to save
        return;
    }
    saveTo(currentFile);
}

void MainWindow::saveFileAs() {
    QString suggestion = currentFile.isEmpty() ? QString("clinic_data.csv") : currentFile;
    QString path = QFileDialog::getSaveFileName(
        this, "Save clinic data", suggestion, "Clinic data (*.csv);;All files (*)");
    if (path.isEmpty()) return;      // the user pressed Cancel
    saveTo(path);
}

void MainWindow::loadFile() {
    QString path = QFileDialog::getOpenFileName(
        this, "Load clinic data", QString(), "Clinic data (*.csv);;All files (*)");
    if (path.isEmpty()) return;

    try {
        clinic.loadFromFile(path.toStdString());
        currentFile = path;
        refreshAll();
        showStatus(QString("Loaded %1 patients from %2")
                       .arg(static_cast<int>(clinic.patientCount()))
                       .arg(path));
    } catch (const utils::ClinicException& e) {
        // A bad file changes nothing: the clinic is exactly as it was
        QMessageBox::critical(this, "Load failed", QString::fromUtf8(e.what()));
    }
}

void MainWindow::undoLast() {
    try {
        std::string what = clinic.undoLastOperation();
        refreshAll();
        showStatus("Undone: " + QString::fromStdString(what));
    } catch (const utils::ClinicException& e) {
        QMessageBox::information(this, "Undo", QString::fromUtf8(e.what()));
    }
}

void MainWindow::focusSearch() {
    tabs->setCurrentWidget(searchPanel);
    searchPanel->focusSearch();
}

void MainWindow::showAbout() {
    QMessageBox::about(this, "About SCAPMS",
        "<h3>SCAPMS</h3>"
        "<p>Smart Clinic Appointment &amp; Patient Management System</p>"
        "<p>COS202 Programming II lab assessment.</p>");
}

} // namespace gui
} // namespace scapms