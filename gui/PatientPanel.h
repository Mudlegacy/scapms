#ifndef PATIENTPANEL_H
#define PATIENTPANEL_H

#include "gui/ClinicPanel.h"
#include "model/PatientData.h"

class QComboBox;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QStackedWidget;
class QTableWidget;

namespace scapms {
namespace gui {

class PatientPanel : public ClinicPanel {
    Q_OBJECT

private:
    QTableWidget* table;

    // Fields shared by every patient type
    QComboBox* typeCombo;
    QLineEdit* nameEdit;
    QLineEdit* ageEdit;
    QLineEdit* contactEdit;

    // One "page" of extra fields per patient type; only one is visible at a time
    QStackedWidget* typeFields;
    QLineEdit* complaintEdit;       // General
    QSpinBox* severitySpin;         // Emergency
    QLineEdit* conditionEdit;       // Emergency
    QLineEdit* diagnosisEdit;       // Follow-Up
    QLineEdit* lastVisitEdit;       // Follow-Up

    QPushButton* addButton;
    QPushButton* updateButton;
    QPushButton* deleteButton;
    QPushButton* clearButton;

    int selectedId;                 // ID of the patient being edited, or -1

    QWidget* buildForm();
    model::PatientData readForm() const;    // throws a ClinicException if the age is not a number
    void fillForm(int id);
    void clearFields();
    void updateButtons();

private slots:
    void onSelectionChanged();
    void addPatient();
    void updatePatient();
    void deletePatient();
    void clearForm();
    void submitForm();              // Enter pressed in a text box

public:
    explicit PatientPanel(controller::ClinicManager& clinic, QWidget* parent = nullptr);
    void refresh() override;
};

} // namespace gui
} // namespace scapms

#endif