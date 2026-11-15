#ifndef QUEUEPANEL_H
#define QUEUEPANEL_H

#include <QDateTime>
#include <QHash>
#include <QSet>
#include <QString>

#include "gui/ClinicPanel.h"

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QTableWidget;
class QTimer;

namespace scapms {
namespace gui {

class QueuePanel : public ClinicPanel {
    Q_OBJECT

private:
    QTableWidget* table;
    QLabel* alertLabel;            // red banner, visible only when someone waits too long
    QLabel* waitingLabel;
    QLabel* nowServingLabel;
    QComboBox* patientCombo;
    QPushButton* addButton;
    QPushButton* callButton;
    QDoubleSpinBox* alertSpin;     // alert threshold in minutes
    QTimer* clockTimer;            // ticks every second

    QString nowServing;                // text of the patient who was called last
    QHash<int, QDateTime> joinedAt;    // patient ID -> when we first saw them waiting
    QSet<int> alerted;                 // patients we have already raised an alert for

    void rebuildTable();
    void rebuildPatientCombo();
    void updateButtons();

private slots:
    void addToQueue();
    void callNext();
    void updateWaitingTimes();

public:
    explicit QueuePanel(controller::ClinicManager& clinic, QWidget* parent = nullptr);
    void refresh() override;
};

} // namespace gui
} // namespace scapms

#endif