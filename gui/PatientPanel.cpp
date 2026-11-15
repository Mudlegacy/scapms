#include "gui/PatientPanel.h"

#include <QBrush>
#include <QColor>
#include <QComboBox>
#include <QFont>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QList>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include "model/PatientFactory.h"
#include "utils/ClinicException.h"
#include "utils/StringUtils.h"

namespace scapms {
namespace gui {

using model::PatientData;
using model::PatientRecord;
using model::PatientType;

namespace {

// Table columns
const int ColId = 0;
const int ColName = 1;
const int ColAge = 2;
const int ColContact = 3;
const int ColCategory = 4;
const int ColPriority = 5;
const int ColDetails = 6;
const int ColumnCount = 7;

// Priority 1 = Emergency, 2 = General, 3 = Follow-Up.
// The alpha value lets the colour blend with the table in light or dark mode.
QColor rowColor(int priority) {
    switch (priority) {
        case 1:  return QColor(231, 76, 60, 90);     // red tint: emergency
        case 3:  return QColor(46, 204, 113, 70);    // green tint: follow-up
        default: return QColor(52, 152, 219, 55);    // blue tint: general
    }
}

QString priorityText(int priority) {
    switch (priority) {
        case 1:  return "High priority";
        case 3:  return "Scheduled";
        default: return "Normal";
    }
}

QString detailsText(const PatientData& d) {
    switch (d.type) {
        case PatientType::General:
            return "Complaint: " + QString::fromStdString(d.complaint);
        case PatientType::Emergency:
            return QString("Severity %1/5 - %2")
                .arg(d.severityLevel)
                .arg(QString::fromStdString(d.condition));
        case PatientType::FollowUp:
            return QString("%1 (last visit %2)")
                .arg(QString::fromStdString(d.previousDiagnosis))
                .arg(QString::fromStdString(d.lastVisitDate));
    }
    return QString();
}

int typeToIndex(PatientType type) {
    switch (type) {
        case PatientType::General:   return 0;
        case PatientType::Emergency: return 1;
        case PatientType::FollowUp:  return 2;
    }
    return 0;
}

// Manual linear search through the list the table shows
const PatientRecord* findInList(const std::vector<PatientRecord*>& list, int id) {
    for (size_t i = 0; i < list.size(); i++) {
        if (list[i]->getId() == id) return list[i];
    }
    return nullptr;
}

} // anonymous namespace

// ============================ construction ============================

PatientPanel::PatientPanel(controller::ClinicManager& clinic, QWidget* parent)
    : ClinicPanel(clinic, parent), selectedId(-1) {

    // ---- The table ----
    table = new QTableWidget(0, ColumnCount);
    QStringList headers;
    headers << "ID" << "Name" << "Age" << "Contact" << "Category" << "Priority" << "Details";
    table->setHorizontalHeaderLabels(headers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);    // editing is done in the form
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setStretchLastSection(true);
    table->setToolTip("Click a row to edit or delete that patient.\n"
                      "Red = emergency, blue = general, green = follow-up.");

    // ---- Layout: table on the left, form on the right ----
    QVBoxLayout* rightSide = new QVBoxLayout();
    rightSide->addWidget(buildForm());
    rightSide->addStretch();

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->addWidget(table, 5);
    layout->addLayout(rightSide, 2);

    // ---- Events ----
    // Choosing a type swaps the extra fields (QStackedWidget shows one page at a time)
    connect(typeCombo, &QComboBox::currentIndexChanged, typeFields, &QStackedWidget::setCurrentIndex);
    connect(table, &QTableWidget::itemSelectionChanged, this, &PatientPanel::onSelectionChanged);

    connect(addButton, &QPushButton::clicked, this, &PatientPanel::addPatient);
    connect(updateButton, &QPushButton::clicked, this, &PatientPanel::updatePatient);
    connect(deleteButton, &QPushButton::clicked, this, &PatientPanel::deletePatient);
    connect(clearButton, &QPushButton::clicked, this, &PatientPanel::clearForm);

    // Text field events: pressing Enter in any box submits the form
    connect(nameEdit, &QLineEdit::returnPressed, this, &PatientPanel::submitForm);
    connect(ageEdit, &QLineEdit::returnPressed, this, &PatientPanel::submitForm);
    connect(contactEdit, &QLineEdit::returnPressed, this, &PatientPanel::submitForm);
    connect(complaintEdit, &QLineEdit::returnPressed, this, &PatientPanel::submitForm);
    connect(conditionEdit, &QLineEdit::returnPressed, this, &PatientPanel::submitForm);
    connect(diagnosisEdit, &QLineEdit::returnPressed, this, &PatientPanel::submitForm);
    connect(lastVisitEdit, &QLineEdit::returnPressed, this, &PatientPanel::submitForm);

    updateButtons();
    refresh();
}

QWidget* PatientPanel::buildForm() {
    // ---- Fields shared by every type ----
    typeCombo = new QComboBox();
    typeCombo->addItem("General");
    typeCombo->addItem("Emergency");
    typeCombo->addItem("Follow-Up");
    typeCombo->setToolTip("The kind of patient. The fields below change to match.");

    nameEdit = new QLineEdit();
    nameEdit->setPlaceholderText("Full name");
    ageEdit = new QLineEdit();
    ageEdit->setPlaceholderText("0 - 120");
    contactEdit = new QLineEdit();
    contactEdit->setPlaceholderText("e.g. 08012345678");
    contactEdit->setToolTip("7 to 15 digits, with an optional leading +");

    // ---- Page 0: General ----
    complaintEdit = new QLineEdit();
    complaintEdit->setPlaceholderText("Reason for the visit");
    QWidget* generalPage = new QWidget();
    QFormLayout* generalLayout = new QFormLayout(generalPage);
    generalLayout->setContentsMargins(0, 0, 0, 0);
    generalLayout->addRow("Complaint:", complaintEdit);

    // ---- Page 1: Emergency ----
    severitySpin = new QSpinBox();
    severitySpin->setRange(1, 5);
    severitySpin->setValue(3);
    severitySpin->setToolTip("1 = mild, 5 = critical");
    conditionEdit = new QLineEdit();
    conditionEdit->setPlaceholderText("What brought them in");
    QWidget* emergencyPage = new QWidget();
    QFormLayout* emergencyLayout = new QFormLayout(emergencyPage);
    emergencyLayout->setContentsMargins(0, 0, 0, 0);
    emergencyLayout->addRow("Severity (1-5):", severitySpin);
    emergencyLayout->addRow("Condition:", conditionEdit);

    // ---- Page 2: Follow-Up ----
    diagnosisEdit = new QLineEdit();
    diagnosisEdit->setPlaceholderText("Previous diagnosis");
    lastVisitEdit = new QLineEdit();
    lastVisitEdit->setPlaceholderText("YYYY-MM-DD");
    QWidget* followUpPage = new QWidget();
    QFormLayout* followUpLayout = new QFormLayout(followUpPage);
    followUpLayout->setContentsMargins(0, 0, 0, 0);
    followUpLayout->addRow("Diagnosis:", diagnosisEdit);
    followUpLayout->addRow("Last visit:", lastVisitEdit);

    // The stack: the page order matches the order of the combo box items
    typeFields = new QStackedWidget();
    typeFields->addWidget(generalPage);
    typeFields->addWidget(emergencyPage);
    typeFields->addWidget(followUpPage);

    // ---- Buttons ----
    addButton = new QPushButton("Add patient");
    addButton->setToolTip("Register a new patient with the details above");
    updateButton = new QPushButton("Save changes");
    updateButton->setToolTip("Save the changes to the selected patient");
    deleteButton = new QPushButton("Delete");
    deleteButton->setToolTip("Delete the selected patient (can be undone from the Edit menu)");
    clearButton = new QPushButton("Clear / New");
    clearButton->setToolTip("Empty the form so you can add a new patient");

    QHBoxLayout* firstRow = new QHBoxLayout();
    firstRow->addWidget(addButton);
    firstRow->addWidget(updateButton);
    QHBoxLayout* secondRow = new QHBoxLayout();
    secondRow->addWidget(deleteButton);
    secondRow->addWidget(clearButton);

    // ---- Put it all together ----
    QGroupBox* box = new QGroupBox("Patient details");
    box->setMinimumWidth(300);
    QFormLayout* form = new QFormLayout(box);
    form->addRow("Type:", typeCombo);
    form->addRow("Name:", nameEdit);
    form->addRow("Age:", ageEdit);
    form->addRow("Contact:", contactEdit);
    form->addRow(typeFields);
    form->addRow(firstRow);
    form->addRow(secondRow);
    return box;
}

// ============================ the form ============================

PatientData PatientPanel::readForm() const {
    PatientData d;
    switch (typeCombo->currentIndex()) {
        case 1:  d.type = PatientType::Emergency; break;
        case 2:  d.type = PatientType::FollowUp;  break;
        default: d.type = PatientType::General;   break;
    }

    d.name = nameEdit->text().trimmed().toStdString();
    d.contact = contactEdit->text().trimmed().toStdString();

    // The only check done here: the age box must hold a whole number.
    // Every other rule (0-120, contact digits, dates...) is checked by the core.
    int age = 0;
    if (!utils::tryParseInt(ageEdit->text().trimmed().toStdString(), age)) {
        throw utils::InvalidDataException("Patient age must be a whole number.");
    }
    d.age = age;

    // Fields of the other types are read too, but the core ignores them
    d.complaint = complaintEdit->text().trimmed().toStdString();
    d.severityLevel = severitySpin->value();
    d.condition = conditionEdit->text().trimmed().toStdString();
    d.previousDiagnosis = diagnosisEdit->text().trimmed().toStdString();
    d.lastVisitDate = lastVisitEdit->text().trimmed().toStdString();
    return d;
}

void PatientPanel::fillForm(int id) {
    const PatientRecord* patient = findInList(clinic.getPatients(), id);
    if (patient == nullptr) return;

    PatientData d = model::PatientFactory::extract(*patient);

    typeCombo->setCurrentIndex(typeToIndex(d.type));   // also switches the visible page
    nameEdit->setText(QString::fromStdString(d.name));
    ageEdit->setText(QString::number(d.age));
    contactEdit->setText(QString::fromStdString(d.contact));
    complaintEdit->setText(QString::fromStdString(d.complaint));
    severitySpin->setValue(d.severityLevel);
    conditionEdit->setText(QString::fromStdString(d.condition));
    diagnosisEdit->setText(QString::fromStdString(d.previousDiagnosis));
    lastVisitEdit->setText(QString::fromStdString(d.lastVisitDate));
}

void PatientPanel::clearFields() {
    nameEdit->clear();
    ageEdit->clear();
    contactEdit->clear();
    complaintEdit->clear();
    severitySpin->setValue(3);
    conditionEdit->clear();
    diagnosisEdit->clear();
    lastVisitEdit->clear();
}

void PatientPanel::updateButtons() {
    bool editing = (selectedId != -1);
    addButton->setEnabled(!editing);
    updateButton->setEnabled(editing);
    deleteButton->setEnabled(editing);
    typeCombo->setEnabled(!editing);    // an existing patient's type cannot be changed
}

// ============================ slots ============================

void PatientPanel::onSelectionChanged() {
    QList<QTableWidgetItem*> items = table->selectedItems();
    if (items.isEmpty()) {
        selectedId = -1;
    } else {
        selectedId = table->item(items.first()->row(), ColId)->text().toInt();
        fillForm(selectedId);
    }
    updateButtons();
}

void PatientPanel::addPatient() {
    try {
        PatientData data = readForm();
        int id = clinic.registerPatient(data);

        QString message = QString("Registered patient ID %1 (%2).")
                              .arg(id)
                              .arg(QString::fromStdString(data.name));
        if (data.type == PatientType::Emergency) {
            message += " Added to the waiting queue.";
        }

        clearForm();
        emit dataChanged();
        emit statusMessage(message);
    } catch (const utils::ClinicException& e) {
        QMessageBox::warning(this, "Cannot add patient", QString::fromUtf8(e.what()));
    }
}

void PatientPanel::updatePatient() {
    if (selectedId == -1) return;
    try {
        PatientData data = readForm();
        clinic.editPatient(selectedId, data);

        emit dataChanged();
        emit statusMessage(QString("Updated patient ID %1. Use Edit > Undo to reverse it.")
                               .arg(selectedId));
    } catch (const utils::ClinicException& e) {
        QMessageBox::warning(this, "Cannot save changes", QString::fromUtf8(e.what()));
    }
}

void PatientPanel::deletePatient() {
    if (selectedId == -1) return;

    QMessageBox::StandardButton answer = QMessageBox::question(
        this, "Delete patient",
        QString("Delete patient ID %1?\nYou can undo this from the Edit menu.").arg(selectedId),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) return;

    try {
        int id = selectedId;
        clinic.deletePatient(id);

        clearForm();
        emit dataChanged();
        emit statusMessage(QString("Deleted patient ID %1. Use Edit > Undo to restore it.").arg(id));
    } catch (const utils::ClinicException& e) {
        QMessageBox::warning(this, "Cannot delete patient", QString::fromUtf8(e.what()));
    }
}

void PatientPanel::clearForm() {
    table->clearSelection();
    selectedId = -1;
    clearFields();
    updateButtons();
    nameEdit->setFocus();
}

void PatientPanel::submitForm() {
    if (selectedId == -1) addPatient();
    else updatePatient();
}

// ============================ refresh ============================

void PatientPanel::refresh() {
    const std::vector<PatientRecord*>& patients = clinic.getPatients();

    // Rebuilding the table would fire selection events that overwrite the form,
    // so the table's signals are paused until we are done.
    QSignalBlocker blocker(table);

    table->setRowCount(0);
    table->setRowCount(static_cast<int>(patients.size()));

    int rowToSelect = -1;
    for (size_t i = 0; i < patients.size(); i++) {
        const PatientRecord& patient = *patients[i];
        PatientData d = model::PatientFactory::extract(patient);
        int row = static_cast<int>(i);
        QColor tint = rowColor(patient.getPriority());

        QStringList cells;
        cells << QString::number(patient.getId())
              << QString::fromStdString(patient.getName())
              << QString::number(patient.getAge())
              << QString::fromStdString(patient.getContact())
              << QString::fromStdString(patient.getCategory())
              << priorityText(patient.getPriority())
              << detailsText(d);

        for (int col = 0; col < ColumnCount; col++) {
            QTableWidgetItem* item = new QTableWidgetItem(cells[col]);
            item->setBackground(QBrush(tint));          // colour by category
            if (col == ColCategory) {
                QFont font = item->font();
                font.setBold(true);
                item->setFont(font);
            }
            table->setItem(row, col, item);
        }

        if (patient.getId() == selectedId) rowToSelect = row;
    }

    table->resizeColumnsToContents();

    if (rowToSelect != -1) {
        table->selectRow(rowToSelect);      // keep the same patient selected
    } else if (selectedId != -1) {
        selectedId = -1;                    // that patient no longer exists
        clearFields();
    }
    updateButtons();
}

} // namespace gui
} // namespace scapms