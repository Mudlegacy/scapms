#ifndef APPOINTMENTPANEL_H
#define APPOINTMENTPANEL_H

#include "gui/ClinicPanel.h"

class QComboBox;
class QDateEdit;
class QLineEdit;
class QPushButton;
class QTableWidget;
class QTimeEdit;

namespace scapms {
namespace gui {

class AppointmentPanel : public ClinicPanel {
    Q_OBJECT

private:
    QTableWidget* table;
    QComboBox* filterCombo;          // All / Pending / Completed / Cancelled

    // The "schedule" form
    QComboBox* patientCombo;
    QDateEdit* dateEdit;
    QTimeEdit* timeEdit;
    QLineEdit* reasonEdit;

    QPushButton* scheduleButton;
    QPushButton* completeButton;
    QPushButton* cancelButton;

    // What is selected in the table
    int selectedPatientId;
    int selectedAppointmentId;
    bool selectedIsPending;

    QWidget* buildSidePanel();
    void rebuildPatientCombo();
    void rebuildTable();
    void updateButtons();

private slots:
    void onSelectionChanged();
    void onFilterChanged();
    void scheduleAppointment();
    void completeAppointment();
    void cancelAppointment();

public:
    explicit AppointmentPanel(controller::ClinicManager& clinic, QWidget* parent = nullptr);
    void refresh() override;
};

} // namespace gui
} // namespace scapms

#endif