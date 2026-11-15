#include "gui/DashboardPanel.h"

#include <ctime>
#include <vector>

#include <QBrush>
#include <QColor>
#include <QFont>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QSpinBox>
#include <QStringList>
#include <QTime>
#include <QTimer>
#include <QVBoxLayout>

#include "controller/ReminderService.h"
#include "utils/ClinicException.h"

namespace scapms {
namespace gui {

namespace {

// A big bold label for a number
QLabel* makeValueLabel() {
    QLabel* label = new QLabel("0");
    QFont font = label->font();
    font.setPointSize(font.pointSize() + 4);
    font.setBold(true);
    label->setFont(font);
    return label;
}

// Manual linear search for a patient's name
QString nameOf(const std::vector<model::PatientRecord*>& list, int patientId) {
    for (size_t i = 0; i < list.size(); i++) {
        if (list[i]->getId() == patientId) return QString::fromStdString(list[i]->getName());
    }
    return "(unknown patient)";
}

QString durationText(int minutes) {
    if (minutes < 60) return QString("%1 min").arg(minutes);
    if (minutes < 24 * 60) return QString("%1 h %2 min").arg(minutes / 60).arg(minutes % 60);
    return QString("%1 day(s)").arg(minutes / (24 * 60));
}

QString reminderText(const controller::Reminder& r, const QString& patientName) {
    QString what = patientName + " - " + QString::fromStdString(r.reason) +
                   " (" + QString::fromStdString(r.date) + " " + QString::fromStdString(r.time) + ")";
    if (r.overdue) return "OVERDUE by " + durationText(r.minutes) + ": " + what;
    if (r.minutes == 0) return "NOW: " + what;
    return "In " + durationText(r.minutes) + ": " + what;
}

} // anonymous namespace

DashboardPanel::DashboardPanel(controller::ClinicManager& clinic, QWidget* parent)
    : ClinicPanel(clinic, parent) {

    totalLabel = makeValueLabel();
    waitingLabel = makeValueLabel();
    pendingLabel = makeValueLabel();
    completedLabel = makeValueLabel();
    cancelledLabel = makeValueLabel();
    undoLabel = makeValueLabel();
    cacheLabel = makeValueLabel();

    totalLabel->setToolTip("All patients currently registered");
    waitingLabel->setToolTip("Patients in the waiting queue right now");
    undoLabel->setToolTip("How many administrative operations can still be undone");
    cacheLabel->setToolTip("How often a patient was found in the recent-patients cache");

    // QFormLayout: neat "label: value" rows inside a group box
    QGroupBox* patientsBox = new QGroupBox("Patients");
    QFormLayout* patientsForm = new QFormLayout(patientsBox);
    patientsForm->addRow("Registered:", totalLabel);
    patientsForm->addRow("Waiting now:", waitingLabel);

    QGroupBox* appointmentsBox = new QGroupBox("Appointments");
    QFormLayout* appointmentsForm = new QFormLayout(appointmentsBox);
    appointmentsForm->addRow("Pending:", pendingLabel);
    appointmentsForm->addRow("Completed:", completedLabel);
    appointmentsForm->addRow("Cancelled:", cancelledLabel);

    QGroupBox* systemBox = new QGroupBox("System");
    QFormLayout* systemForm = new QFormLayout(systemBox);
    systemForm->addRow("Undo available:", undoLabel);
    systemForm->addRow("Recent cache:", cacheLabel);

    // QHBoxLayout: the three boxes side by side
    QHBoxLayout* boxes = new QHBoxLayout();
    boxes->addWidget(patientsBox);
    boxes->addWidget(appointmentsBox);
    boxes->addWidget(systemBox);

    // ---- Reminders ----
    windowSpin = new QSpinBox();
    windowSpin->setRange(1, 1440);
    windowSpin->setValue(30);
    windowSpin->setSuffix(" min");
    windowSpin->setToolTip("A pop-up appears when a pending appointment starts within this time");

    QHBoxLayout* windowRow = new QHBoxLayout();
    windowRow->addWidget(new QLabel("Remind me when an appointment is due within:"));
    windowRow->addWidget(windowSpin);
    windowRow->addStretch();

    reminderList = new QListWidget();
    reminderList->setToolTip("Orange = due soon, red = overdue (still pending)");

    QGroupBox* remindersBox = new QGroupBox("Appointment reminders");
    QVBoxLayout* remindersLayout = new QVBoxLayout(remindersBox);
    remindersLayout->addLayout(windowRow);
    remindersLayout->addWidget(reminderList);

    updatedLabel = new QLabel();

    QLabel* title = new QLabel("Clinic dashboard");
    QFont titleFont = title->font();
    titleFont.setPointSize(titleFont.pointSize() + 8);
    titleFont.setBold(true);
    title->setFont(titleFont);

    // QVBoxLayout: title on top, boxes below, then the reminders
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(title);
    layout->addLayout(boxes);
    layout->addWidget(remindersBox, 1);
    layout->addWidget(updatedLabel);

    // ---- Timers ----
    refreshTimer = new QTimer(this);              // 'this' owns it and deletes it for us
    refreshTimer->setInterval(5000);              // every 5 seconds
    connect(refreshTimer, &QTimer::timeout, this, &DashboardPanel::refresh);
    refreshTimer->start();

    reminderTimer = new QTimer(this);
    reminderTimer->setInterval(15000);            // every 15 seconds
    connect(reminderTimer, &QTimer::timeout, this, &DashboardPanel::checkReminders);
    reminderTimer->start();

    connect(windowSpin, &QSpinBox::valueChanged, this, &DashboardPanel::fillReminders);

    refresh();
}

void DashboardPanel::refresh() {
    controller::DashboardStats stats = clinic.getStats();

    totalLabel->setText(QString::number(stats.totalPatients));
    waitingLabel->setText(QString::number(stats.waitingCount));
    pendingLabel->setText(QString::number(stats.pendingAppointments));
    completedLabel->setText(QString::number(stats.completedAppointments));
    cancelledLabel->setText(QString::number(stats.cancelledAppointments));
    undoLabel->setText(QString::number(stats.undoDepth));
    cacheLabel->setText(QString("%1 hits / %2 misses")
                            .arg(stats.cacheHits)
                            .arg(stats.cacheMisses));

    fillReminders();
    updatedLabel->setText("Last updated: " + QTime::currentTime().toString("HH:mm:ss") +
                          "  (refreshes every 5 seconds)");
}

// The list on the dashboard: everything due soon, and everything overdue
void DashboardPanel::fillReminders() {
    reminderList->clear();

    try {
        std::vector<controller::Reminder> reminders = controller::ReminderService::findReminders(
            clinic.getAllAppointments(), std::time(nullptr), windowSpin->value());
        const std::vector<model::PatientRecord*>& patients = clinic.getPatients();

        if (reminders.empty()) {
            reminderList->addItem("Nothing due soon, and nothing overdue.");
            return;
        }

        for (size_t i = 0; i < reminders.size(); i++) {
            const controller::Reminder& r = reminders[i];
            QListWidgetItem* item = new QListWidgetItem(reminderText(r, nameOf(patients, r.patientId)));
            item->setForeground(QBrush(r.overdue ? QColor(231, 76, 60) : QColor(230, 126, 34)));
            reminderList->addItem(item);
        }
    } catch (const utils::ClinicException& e) {
        reminderList->addItem("Cannot check reminders: " + QString::fromUtf8(e.what()));
    }
}

// Runs every 15 seconds: warns ONCE about each appointment that is about to start
void DashboardPanel::checkReminders() {
    try {
        std::vector<controller::Reminder> reminders = controller::ReminderService::findReminders(
            clinic.getAllAppointments(), std::time(nullptr), windowSpin->value());
        const std::vector<model::PatientRecord*>& patients = clinic.getPatients();

        QStringList lines;
        for (size_t i = 0; i < reminders.size(); i++) {
            const controller::Reminder& r = reminders[i];
            if (r.overdue) continue;        // overdue ones stay on the dashboard list, no pop-up

            QString key = QString("%1|%2|%3")
                              .arg(r.appointmentId)
                              .arg(QString::fromStdString(r.date))
                              .arg(QString::fromStdString(r.time));
            if (reminded.contains(key)) continue;     // already warned about this one
            reminded.insert(key);

            lines << reminderText(r, nameOf(patients, r.patientId));
        }
        if (lines.isEmpty()) return;

        emit statusMessage(QString("Reminder: %1 appointment(s) starting soon.").arg(lines.size()));

        // A non-modal box: staff can keep working while it stays open
        QMessageBox* box = new QMessageBox(QMessageBox::Information, "Appointment reminder",
                                           lines.join("\n"), QMessageBox::Ok, this);
        box->setWindowModality(Qt::NonModal);
        box->setAttribute(Qt::WA_DeleteOnClose);      // freed automatically when closed
        box->show();
    } catch (const utils::ClinicException&) {
        // A reminder problem must never crash the program. Try again on the next tick.
    }
}

} // namespace gui
} // namespace scapms