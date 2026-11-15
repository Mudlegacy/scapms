#ifndef DASHBOARDPANEL_H
#define DASHBOARDPANEL_H

#include <QSet>
#include <QString>

#include "gui/ClinicPanel.h"

class QLabel;
class QListWidget;
class QSpinBox;
class QTimer;

namespace scapms {
namespace gui {

class DashboardPanel : public ClinicPanel {
    Q_OBJECT

private:
    QLabel* totalLabel;
    QLabel* waitingLabel;
    QLabel* pendingLabel;
    QLabel* completedLabel;
    QLabel* cancelledLabel;
    QLabel* undoLabel;
    QLabel* cacheLabel;
    QLabel* updatedLabel;

    QSpinBox* windowSpin;           // how many minutes ahead to warn
    QListWidget* reminderList;

    QTimer* refreshTimer;           // redraws the numbers every few seconds
    QTimer* reminderTimer;          // looks for appointments that are about to start
    QSet<QString> reminded;         // appointments we have already warned about

    void fillReminders();

private slots:
    void checkReminders();

public:
    explicit DashboardPanel(controller::ClinicManager& clinic, QWidget* parent = nullptr);
    void refresh() override;
};

} // namespace gui
} // namespace scapms

#endif