#include "gui/QueuePanel.h"

#include <vector>

#include <QBrush>
#include <QChar>
#include <QColor>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFont>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QList>
#include <QMessageBox>
#include <QPushButton>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QVBoxLayout>

#include "gui/PatientStyle.h"
#include "utils/ClinicException.h"

namespace scapms {
namespace gui {

using model::PatientRecord;

namespace {

// Table columns
const int ColId = 1;
const int ColName = 2;
const int ColWaiting = 6;
const int ColumnCount = 7;

QString patientLabel(const PatientRecord& patient) {
    return QString("ID %1 - %2 (%3)")
        .arg(patient.getId())
        .arg(QString::fromStdString(patient.getName()))
        .arg(QString::fromStdString(patient.getCategory()));
}

} // anonymous namespace

// ============================ construction ============================

QueuePanel::QueuePanel(controller::ClinicManager& clinic, QWidget* parent)
    : ClinicPanel(clinic, parent) {

    // ---- Alert banner (hidden until someone waits too long) ----
    alertLabel = new QLabel();
    alertLabel->setStyleSheet("color: #e74c3c; font-weight: bold;");
    alertLabel->setWordWrap(true);
    alertLabel->setVisible(false);

    // ---- "Now serving" banner ----
    nowServingLabel = new QLabel();
    QFont bigFont = nowServingLabel->font();
    bigFont.setPointSize(bigFont.pointSize() + 6);
    bigFont.setBold(true);
    nowServingLabel->setFont(bigFont);

    QGroupBox* servingBox = new QGroupBox("Now serving");
    QVBoxLayout* servingLayout = new QVBoxLayout(servingBox);
    servingLayout->addWidget(nowServingLabel);

    // ---- The queue table (read-only) ----
    waitingLabel = new QLabel();

    table = new QTableWidget(0, ColumnCount);
    QStringList headers;
    headers << "Position" << "ID" << "Name" << "Age" << "Category" << "Priority" << "Waiting";
    table->setHorizontalHeaderLabels(headers);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setStretchLastSection(true);
    table->setToolTip("Patients in the order they will be called.\n"
                      "Emergency patients go first; everyone else is first come, first served.\n"
                      "The Waiting column counts minutes:seconds.");

    QVBoxLayout* leftSide = new QVBoxLayout();
    leftSide->addWidget(waitingLabel);
    leftSide->addWidget(table);

    // ---- Controls ----
    patientCombo = new QComboBox();
    patientCombo->setToolTip("Patients who are not in the queue yet");
    addButton = new QPushButton("Add to queue");
    addButton->setToolTip("Put the chosen patient at the back of the queue");

    QGroupBox* addBox = new QGroupBox("Add a patient to the queue");
    QFormLayout* addForm = new QFormLayout(addBox);
    addForm->addRow("Patient:", patientCombo);
    addForm->addRow(addButton);

    callButton = new QPushButton("Call next patient");
    callButton->setToolTip("Take the patient at the top of the queue for consultation");

    QGroupBox* callBox = new QGroupBox("Consultation");
    QVBoxLayout* callLayout = new QVBoxLayout(callBox);
    callLayout->addWidget(callButton);

    alertSpin = new QDoubleSpinBox();
    alertSpin->setRange(0.1, 240.0);
    alertSpin->setDecimals(1);
    alertSpin->setSingleStep(0.5);
    alertSpin->setValue(10.0);
    alertSpin->setSuffix(" min");
    alertSpin->setToolTip("Warn staff when a patient has waited longer than this.\n"
                          "Set it low (0.1 = 6 seconds) to see the alert quickly.");

    QGroupBox* alertBox = new QGroupBox("Waiting-time alert");
    QFormLayout* alertForm = new QFormLayout(alertBox);
    alertForm->addRow("Alert after:", alertSpin);

    QVBoxLayout* rightSide = new QVBoxLayout();
    rightSide->addWidget(addBox);
    rightSide->addWidget(callBox);
    rightSide->addWidget(alertBox);
    rightSide->addStretch();

    // ---- Page layout: banners on top, table and controls below ----
    QHBoxLayout* lower = new QHBoxLayout();
    lower->addLayout(leftSide, 5);
    lower->addLayout(rightSide, 2);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(alertLabel);
    layout->addWidget(servingBox);
    layout->addLayout(lower);

    // ---- Events ----
    connect(addButton, &QPushButton::clicked, this, &QueuePanel::addToQueue);
    connect(callButton, &QPushButton::clicked, this, &QueuePanel::callNext);

    // A new threshold: forget old alerts and check everyone again straight away
    connect(alertSpin, &QDoubleSpinBox::valueChanged, this, [this]() {
        alerted.clear();
        updateWaitingTimes();
    });

    // ---- Timer: once a second the Waiting column is redrawn ----
    clockTimer = new QTimer(this);
    clockTimer->setInterval(1000);
    connect(clockTimer, &QTimer::timeout, this, &QueuePanel::updateWaitingTimes);
    clockTimer->start();

    refresh();
}

// ============================ rebuilding the widgets ============================

void QueuePanel::rebuildTable() {
    std::vector<PatientRecord*> waiting = clinic.getWaitingPatients();   // in call order

    // Note when each patient was first seen waiting
    QDateTime now = QDateTime::currentDateTime();
    for (size_t i = 0; i < waiting.size(); i++) {
        if (!joinedAt.contains(waiting[i]->getId())) {
            joinedAt.insert(waiting[i]->getId(), now);
        }
    }

    // ... and forget the patients who have left the queue
    QList<int> known = joinedAt.keys();
    for (int k = 0; k < known.size(); k++) {
        bool stillWaiting = false;
        for (size_t j = 0; j < waiting.size(); j++) {
            if (waiting[j]->getId() == known[k]) {
                stillWaiting = true;
                break;
            }
        }
        if (!stillWaiting) {
            joinedAt.remove(known[k]);
            alerted.remove(known[k]);
        }
    }

    table->setRowCount(static_cast<int>(waiting.size()));
    for (size_t i = 0; i < waiting.size(); i++) {
        const PatientRecord& patient = *waiting[i];
        int row = static_cast<int>(i);
        QColor tint = priorityColor(patient.getPriority());

        QStringList cells;
        cells << QString::number(row + 1)
              << QString::number(patient.getId())
              << QString::fromStdString(patient.getName())
              << QString::number(patient.getAge())
              << QString::fromStdString(patient.getCategory())
              << priorityLabel(patient.getPriority())
              << "0:00";                                  // updateWaitingTimes() fills this in

        for (int col = 0; col < cells.size(); col++) {
            QTableWidgetItem* item = new QTableWidgetItem(cells[col]);
            item->setBackground(QBrush(tint));
            table->setItem(row, col, item);
        }
    }
    table->resizeColumnsToContents();

    if (waiting.empty()) {
        waitingLabel->setText("Nobody is waiting.");
    } else {
        waitingLabel->setText(QString("%1 patient(s) waiting. The next one to be called is at the top.")
                                  .arg(static_cast<int>(waiting.size())));
    }
}

void QueuePanel::rebuildPatientCombo() {
    int previous = (patientCombo->count() > 0) ? patientCombo->currentData().toInt() : -1;

    std::vector<PatientRecord*> waiting = clinic.getWaitingPatients();
    const std::vector<PatientRecord*>& patients = clinic.getPatients();

    patientCombo->clear();
    for (size_t i = 0; i < patients.size(); i++) {
        // Manual check: skip anyone who is already in the queue
        bool alreadyWaiting = false;
        for (size_t j = 0; j < waiting.size(); j++) {
            if (waiting[j]->getId() == patients[i]->getId()) {
                alreadyWaiting = true;
                break;
            }
        }
        if (alreadyWaiting) continue;
        patientCombo->addItem(patientLabel(*patients[i]), patients[i]->getId());
    }

    int index = patientCombo->findData(previous);
    if (index >= 0) patientCombo->setCurrentIndex(index);
}

void QueuePanel::updateButtons() {
    addButton->setEnabled(patientCombo->count() > 0);
    callButton->setEnabled(clinic.waitingCount() > 0);
}

void QueuePanel::refresh() {
    rebuildTable();
    rebuildPatientCombo();
    updateButtons();
    updateWaitingTimes();

    nowServingLabel->setText(nowServing.isEmpty() ? QString("Nobody has been called yet.") : nowServing);
}

// ============================ the timer slot ============================

// Runs every second. Redraws the Waiting column and raises alerts.
void QueuePanel::updateWaitingTimes() {
    QDateTime now = QDateTime::currentDateTime();
    qint64 limitSeconds = static_cast<qint64>(alertSpin->value() * 60.0);
    QStringList overdueNames;

    for (int row = 0; row < table->rowCount(); row++) {
        QTableWidgetItem* idItem = table->item(row, ColId);
        QTableWidgetItem* nameItem = table->item(row, ColName);
        QTableWidgetItem* waitItem = table->item(row, ColWaiting);
        if (idItem == nullptr || nameItem == nullptr || waitItem == nullptr) continue;

        int id = idItem->text().toInt();
        if (!joinedAt.contains(id)) continue;

        qint64 seconds = joinedAt.value(id).secsTo(now);
        if (seconds < 0) seconds = 0;

        QString waited = QString("%1:%2").arg(seconds / 60).arg(seconds % 60, 2, 10, QChar('0'));
        waitItem->setText(waited);

        bool tooLong = (seconds >= limitSeconds);
        QFont font = waitItem->font();
        font.setBold(tooLong);
        waitItem->setFont(font);
        waitItem->setForeground(tooLong ? QBrush(QColor(231, 76, 60)) : QBrush());

        if (tooLong) {
            overdueNames << nameItem->text();
            if (!alerted.contains(id)) {               // announce each patient only once
                alerted.insert(id);
                emit statusMessage(QString("Waiting alert: %1 has been waiting for %2.")
                                       .arg(nameItem->text())
                                       .arg(waited));
            }
        }
    }

    if (overdueNames.isEmpty()) {
        alertLabel->setVisible(false);
    } else {
        alertLabel->setText("ALERT - waiting too long: " + overdueNames.join(", "));
        alertLabel->setVisible(true);
    }
}

// ============================ slots ============================

void QueuePanel::addToQueue() {
    if (patientCombo->count() == 0) return;

    try {
        int patientId = patientCombo->currentData().toInt();
        clinic.addToQueue(patientId);

        emit dataChanged();
        emit statusMessage(QString("Patient ID %1 added to the waiting queue.").arg(patientId));
    } catch (const utils::ClinicException& e) {
        QMessageBox::warning(this, "Cannot add to queue", QString::fromUtf8(e.what()));
    }
}

void QueuePanel::callNext() {
    try {
        PatientRecord* patient = clinic.callNextPatient();

        nowServing = patientLabel(*patient);     // remembered, so refresh() can show it
        QString name = QString::fromStdString(patient->getName());

        emit dataChanged();
        emit statusMessage("Called " + name + ".");
    } catch (const utils::ClinicException& e) {
        QMessageBox::information(this, "Waiting queue", QString::fromUtf8(e.what()));
    }
}

} // namespace gui
} // namespace scapms