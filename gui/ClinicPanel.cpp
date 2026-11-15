#include "gui/ClinicPanel.h"

namespace scapms {
namespace gui {

ClinicPanel::ClinicPanel(controller::ClinicManager& clinic, QWidget* parent)
    : QWidget(parent), clinic(clinic) {}

} // namespace gui
} // namespace scapms