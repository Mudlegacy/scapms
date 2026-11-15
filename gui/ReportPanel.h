#ifndef REPORTPANEL_H
#define REPORTPANEL_H

#include <QString>

#include "gui/ClinicPanel.h"

class QComboBox;
class QDateEdit;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;

namespace scapms {
namespace gui {

class ReportPanel : public ClinicPanel {
    Q_OBJECT

private:
    QComboBox* reportCombo;
    QLabel* topLabel;
    QSpinBox* topSpin;
    QLabel* dateLabel;
    QDateEdit* dateEdit;
    QPushButton* generateButton;
    QPushButton* exportButton;
    QPlainTextEdit* output;

    QString buildReport() const;

private slots:
    void onReportChanged();
    void generate();
    void exportReport();

public:
    explicit ReportPanel(controller::ClinicManager& clinic, QWidget* parent = nullptr);
    void refresh() override;
};

} // namespace gui
} // namespace scapms

#endif