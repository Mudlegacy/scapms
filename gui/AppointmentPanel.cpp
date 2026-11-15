#include "gui/AppointmentPanel.h"

#include <vector>

#include <QBrush>
#include <QColor>
#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QFont>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTime>
#include <QTimeEdit>
#include <QVBoxLayout>

#include "model/Appointable.h"
#include "model/Appointment.h"
#include "utils/ClinicException.h"

namespace scapms {
namespace gui {

using model::Appointment;
using model::AppointmentStatus;
using model::PatientRecord;

namespace {

// Table columns
const int ColAppointment = 0;
const int ColPatientId = 1;
const int ColStatus = 6;
const int ColumnCount = 7;

// The alpha value lets the colour blend with the table in light or dark mode
QColor statusColor(AppointmentStatus status) {
    switch (status) {
        case AppointmentStatus::Pending:   return QColor(241, 196, 15, 75);    // amber
        case AppointmentStatus::Completed: return QColor(46, 204, 113, 75);    // green
        case AppointmentStatus::Cancelled: return QColor(149, 165, 166, 75);   // grey
    }
    return QColor();
}

// Manual linear search for a patient's name
QString nameOf(const std::vector<PatientRecord*>& list, int patientId) {
    for (size_t i = 0; i < list.size(); i++) {
        if (list[i]->getId() == patientId) return QString::fromStdString(list[i]->getName());
    }
    return "(unknown)";
}

QString patientLabel(const PatientRecord& patient) {
    return QString("ID %1 - %2 (%3)")
        .arg(patient.getId())
        .arg(QString::fromStdString(patient.getName()))
        .arg(QString::fromStdString(patient.getCategory()));
}

} // anonymous namespace

// ============================ construction ============================

AppointmentPanel::AppointmentPanel(controller::ClinicManager& clinic, QWidget* parent)
    : ClinicPanel(clinic, parent),
      selectedPatientId(-1), selectedAppointmentId(-1), selectedIsPending(false) {

    // ---- The table ----
    table = new QTableWidget(0, ColumnCount);
    QStringList headers;
    headers << "Appt #" << "Patient ID" << "Patient" << "Date" << "Time" << "Reason" << "Status";
    table->setHorizontalHeaderLabels(headers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setStretchLastSection(true);
    table->setToolTip("Click an appointment to complete or cancel it.\n"
                      "Amber = pending, green = completed, grey = cancelled.");

    // ---- Status filter ----
    filterCombo = new QComboBox();
    filterCombo->addItem("All appointments");
    filterCombo->addItem("Pending");
    filterCombo->addItem("Completed");
    filterCombo->addItem("Cancelled");
    filterCombo->setToolTip("Show only appointments with this status");

    QHBoxLayout* filterRow = new QHBoxLayout();
    filterRow->addWidget(new QLabel("Show:"));
    filterRow->addWidget(filterCombo);
    filterRow->addStretch();

    QVBoxLayout* leftSide = new QVBoxLayout();
    leftSide->addLayout(filterRow);
    leftSide->addWidget(table);

    QVBoxLayout* rightSide = new QVBoxLayout();
    rightSide->addWidget(buildSidePanel());
    rightSide->addStretch();

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->addLayout(leftSide, 5);
    layout->addLayout(rightSide, 2);

    // ---- Events ----
    connect(table, &QTableWidget::itemSelectionChanged, this, &AppointmentPanel::onSelectionChanged);
    connect(filterCombo, &QComboBox::currentIndexChanged, this, &AppointmentPanel::onFilterChanged);
    connect(scheduleButton, &QPushButton::clicked, this, &AppointmentPanel::scheduleAppointment);
    connect(completeButton, &QPushButton::clicked, this, &AppointmentPanel::completeAppointment);
    connect(cancelButton, &QPushButton::clicked, this, &AppointmentPanel::cancelAppointment);
    connect(reasonEdit, &QLineEdit::returnPressed, this, &AppointmentPanel::scheduleAppointment);

    refresh();
}

QWidget* AppointmentPanel::buildSidePanel() {
    // ---- Schedule form ----
    patientCombo = new QComboBox();
    patientCombo->setToolTip("Only General and Follow-Up patients can be scheduled.\n"
                             "Emergency patients are walk-ins and use the waiting queue.");

    dateEdit = new QDateEdit(QDate::currentDate());
    dateEdit->setCalendarPopup(true);                 // click the arrow to get a calendar
    dateEdit->setDisplayFormat("yyyy-MM-dd");

    timeEdit = new QTimeEdit(QTime(9, 0));
    timeEdit->setDisplayFormat("HH:mm");

    reasonEdit = new QLineEdit();
    reasonEdit->setPlaceholderText("Reason for the appointment");

    scheduleButton = new QPushButton("Schedule appointment");
    scheduleButton->setToolTip("Book this appointment for the chosen patient");

    QGroupBox* scheduleBox = new QGroupBox("Schedule an appointment");
    QFormLayout* form = new QFormLayout(scheduleBox);
    form->addRow("Patient:", patientCombo);
    form->addRow("Date:", dateEdit);
    form->addRow("Time:", timeEdit);
    form->addRow("Reason:", reasonEdit);
    form->addRow(scheduleButton);

    // ---- Actions on the selected appointment ----
    completeButton = new QPushButton("Mark as completed");
    completeButton->setToolTip("The patient attended this appointment");
    cancelButton = new QPushButton("Cancel appointment");
    cancelButton->setToolTip("Cancel the selected pending appointment (can be undone from the Edit menu)");

    QGroupBox* selectedBox = new QGroupBox("Selected appointment");
    QVBoxLayout* selectedLayout = new QVBoxLayout(selectedBox);
    selectedLayout->addWidget(completeButton);
    selectedLayout->addWidget(cancelButton);

    QWidget* container = new QWidget();
    container->setMinimumWidth(300);
    QVBoxLayout* containerLayout = new QVBoxLayout(container);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->addWidget(scheduleBox);
    containerLayout->addWidget(selectedBox);
    return container;
}

// ============================ rebuilding the widgets ============================

void AppointmentPanel::rebuildPatientCombo() {
    int previous = (patientCombo->count() > 0) ? patientCombo->currentData().toInt() : -1;

    patientCombo->clear();
    const std::vector<PatientRecord*>& patients = clinic.getPatients();
    for (size_t i = 0; i < patients.size(); i++) {
        // Only patients who implement Appointable can be scheduled
        if (dynamic_cast<const model::Appointable*>(patients[i]) == nullptr) continue;
        patientCombo->addItem(patientLabel(*patients[i]), patients[i]->getId());
    }

    int index = patientCombo->findData(previous);     // keep the same patient chosen
    if (index >= 0) patientCombo->setCurrentIndex(index);
}

void AppointmentPanel::rebuildTable() {
    std::vector<Appointment> all = clinic.getAllAppointments();
    const std::vector<PatientRecord*>& patients = clinic.getPatients();
    int filter = filterCombo->currentIndex();    // 0 all, 1 pending, 2 completed, 3 cancelled

    QSignalBlocker blocker(table);               // no selection events while rebuilding
    table->setRowCount(0);

    int rowToSelect = -1;
    for (size_t i = 0; i < all.size(); i++) {
        const Appointment& appointment = all[i];

        if (filter == 1 && appointment.getStatus() != AppointmentStatus::Pending) continue;
        if (filter == 2 && appointment.getStatus() != AppointmentStatus::Completed) continue;
        if (filter == 3 && appointment.getStatus() != AppointmentStatus::Cancelled) continue;

        int row = table->rowCount();
        table->insertRow(row);
        QColor tint = statusColor(appointment.getStatus());

        QStringList cells;
        cells << QString::number(appointment.getAppointmentId())
              << QString::number(appointment.getPatientId())
              << nameOf(patients, appointment.getPatientId())
              << QString::fromStdString(appointment.getDate())
              << QString::fromStdString(appointment.getTime())
              << QString::fromStdString(appointment.getReason())
              << QString::fromStdString(model::statusToString(appointment.getStatus()));

        for (int col = 0; col < ColumnCount; col++) {
            QTableWidgetItem* item = new QTableWidgetItem(cells[col]);
            item->setBackground(QBrush(tint));                // colour by status
            QFont font = item->font();
            if (col == ColStatus) font.setBold(true);
            if (appointment.getStatus() == AppointmentStatus::Cancelled) font.setItalic(true);
            item->setFont(font);
            table->setItem(row, col, item);
        }

        if (appointment.getAppointmentId() == selectedAppointmentId) {
            rowToSelect = row;
            selectedPatientId = appointment.getPatientId();
            selectedIsPending = appointment.isPending();      // its status may have changed
        }
    }

    table->resizeColumnsToContents();

    if (rowToSelect != -1) {
        table->selectRow(rowToSelect);                        // keep the same row selected
    } else {
        selectedPatientId = -1;                               // it is gone, or filtered out
        selectedAppointmentId = -1;
        selectedIsPending = false;
    }
}

void AppointmentPanel::updateButtons() {
    scheduleButton->setEnabled(patientCombo->count() > 0);

    // Only a pending appointment can be completed or cancelled
    bool canChange = (selectedAppointmentId != -1) && selectedIsPending;
    completeButton->setEnabled(canChange);
    cancelButton->setEnabled(canChange);
}

void AppointmentPanel::refresh() {
    rebuildPatientCombo();
    rebuildTable();
    updateButtons();
}

// ============================ slots ============================

void AppointmentPanel::onSelectionChanged() {
    QList<QTableWidgetItem*> items = table->selectedItems();
    if (items.isEmpty()) {
        selectedPatientId = -1;
        selectedAppointmentId = -1;
        selectedIsPending = false;
    } else {
        int row = items.first()->row();
        selectedAppointmentId = table->item(row, ColAppointment)->text().toInt();
        selectedPatientId = table->item(row, ColPatientId)->text().toInt();
        selectedIsPending = (table->item(row, ColStatus)->text() == "Pending");
    }
    updateButtons();
}

void AppointmentPanel::onFilterChanged() {
    rebuildTable();
    updateButtons();
}

void AppointmentPanel::scheduleAppointment() {
    if (patientCombo->count() == 0) return;

    try {
        int patientId = patientCombo->currentData().toInt();
        std::string date = dateEdit->date().toString(Qt::ISODate).toStdString();
        std::string timeText = timeEdit->time().toString("HH:mm").toStdString();
        std::string reason = reasonEdit->text().trimmed().toStdString();

        int id = clinic.scheduleAppointment(patientId, date, timeText, reason);

        reasonEdit->clear();
        emit dataChanged();
        emit statusMessage(QString("Scheduled appointment #%1 for %2 at %3.")
                               .arg(id)
                               .arg(QString::fromStdString(date))
                               .arg(QString::fromStdString(timeText)));
    } catch (const utils::ClinicException& e) {
        QMessageBox::warning(this, "Cannot schedule appointment", QString::fromUtf8(e.what()));
    }
}

void AppointmentPanel::completeAppointment() {
    if (selectedAppointmentId == -1) return;

    try {
        int appointmentId = selectedAppointmentId;
        clinic.completeAppointment(selectedPatientId, appointmentId);

        emit dataChanged();
        emit statusMessage(QString("Appointment #%1 marked as completed.").arg(appointmentId));
    } catch (const utils::ClinicException& e) {
        QMessageBox::warning(this, "Cannot complete appointment", QString::fromUtf8(e.what()));
    }
}

void AppointmentPanel::cancelAppointment() {
    if (selectedAppointmentId == -1) return;

    try {
        int appointmentId = selectedAppointmentId;
        clinic.cancelAppointment(selectedPatientId, appointmentId);

        emit dataChanged();
        emit statusMessage(QString("Cancelled appointment #%1. Use Edit > Undo to reverse it.")
                               .arg(appointmentId));
    } catch (const utils::ClinicException& e) {
        QMessageBox::warning(this, "Cannot cancel appointment", QString::fromUtf8(e.what()));
    }
}

} // namespace gui
} // namespace scapms