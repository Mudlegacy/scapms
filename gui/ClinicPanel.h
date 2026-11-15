#ifndef CLINICPANEL_H
#define CLINICPANEL_H

#include <QString>
#include <QWidget>

#include "controller/ClinicManager.h"

namespace scapms {
namespace gui {

// Abstract base class for every tab. A panel never owns clinic data:
// it shows it, and sends the user's requests to the ClinicManager.
class ClinicPanel : public QWidget {
    Q_OBJECT

protected:
    controller::ClinicManager& clinic;

public:
    explicit ClinicPanel(controller::ClinicManager& clinic, QWidget* parent = nullptr);
    ~ClinicPanel() override = default;

    // Re-reads the data from the ClinicManager and redraws this panel
    virtual void refresh() = 0;

signals:
    void dataChanged();                          // "I changed something: everyone refresh"
    void statusMessage(const QString& text);     // text for the status bar
};

} // namespace gui
} // namespace scapms

#endif