#ifndef PATIENTSTYLE_H
#define PATIENTSTYLE_H

#include <QColor>
#include <QString>

namespace scapms {
namespace gui {

// Priority 1 = Emergency, 2 = General, 3 = Follow-Up.
// The alpha value lets the colour blend with the table in light or dark mode.
inline QColor priorityColor(int priority) {
    switch (priority) {
        case 1:  return QColor(231, 76, 60, 90);     // red tint: emergency
        case 3:  return QColor(46, 204, 113, 70);    // green tint: follow-up
        default: return QColor(52, 152, 219, 55);    // blue tint: general
    }
}

inline QString priorityLabel(int priority) {
    switch (priority) {
        case 1:  return "High priority";
        case 3:  return "Scheduled";
        default: return "Normal";
    }
}

} // namespace gui
} // namespace scapms

#endif