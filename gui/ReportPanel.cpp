#include "gui/ReportPanel.h"

#include <string>

#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QFile>
#include <QFileDialog>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTextStream>
#include <QVBoxLayout>

#include "utils/ClinicException.h"

namespace scapms {
namespace gui {

ReportPanel::ReportPanel(controller::ClinicManager& clinic, QWidget* parent)
    : ClinicPanel(clinic, parent) {

    reportCombo = new QComboBox();
    reportCombo->addItem("Most frequently attended patients");
    reportCombo->addItem("Patients with pending appointments");
    reportCombo->addItem("Patient category distribution");
    reportCombo->addItem("Daily appointment summary");
    reportCombo->addItem("All reports");
    reportCombo->setToolTip("Choose which report to show");

    topLabel = new QLabel("Show top:");
    topSpin = new QSpinBox();
    topSpin->setRange(1, 50);
    topSpin->setValue(5);
    topSpin->setToolTip("How many patients to list");

    dateLabel = new QLabel("Date:");
    dateEdit = new QDateEdit(QDate::currentDate());
    dateEdit->setCalendarPopup(true);
    dateEdit->setDisplayFormat("yyyy-MM-dd");
    dateEdit->setToolTip("The day to summarise");

    generateButton = new QPushButton("Generate");
    generateButton->setToolTip("Build the report again from the current data");
    exportButton = new QPushButton("Export to file...");
    exportButton->setToolTip("Save the report shown below as a text file");

    output = new QPlainTextEdit();
    output->setReadOnly(true);
    output->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));   // keeps columns lined up

    QHBoxLayout* controls = new QHBoxLayout();
    controls->addWidget(new QLabel("Report:"));
    controls->addWidget(reportCombo);
    controls->addWidget(topLabel);
    controls->addWidget(topSpin);
    controls->addWidget(dateLabel);
    controls->addWidget(dateEdit);
    controls->addStretch();
    controls->addWidget(generateButton);
    controls->addWidget(exportButton);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addLayout(controls);
    layout->addWidget(output);

    // ---- Events ----
    connect(reportCombo, &QComboBox::currentIndexChanged, this, &ReportPanel::onReportChanged);
    connect(topSpin, &QSpinBox::valueChanged, this, &ReportPanel::generate);
    connect(dateEdit, &QDateEdit::dateChanged, this, &ReportPanel::generate);
    connect(generateButton, &QPushButton::clicked, this, &ReportPanel::generate);
    connect(exportButton, &QPushButton::clicked, this, &ReportPanel::exportReport);

    onReportChanged();
}

QString ReportPanel::buildReport() const {
    std::string date = dateEdit->date().toString(Qt::ISODate).toStdString();
    int topN = topSpin->value();
    std::string text;

    switch (reportCombo->currentIndex()) {
        case 0:  text = clinic.reportMostAttended(topN); break;
        case 1:  text = clinic.reportPending(); break;
        case 2:  text = clinic.reportCategories(); break;
        case 3:  text = clinic.reportDaily(date); break;
        default:
            text = clinic.reportMostAttended(topN) + "\n" +
                   clinic.reportPending() + "\n" +
                   clinic.reportCategories() + "\n" +
                   clinic.reportDaily(date);
            break;
    }
    return QString::fromStdString(text);
}

void ReportPanel::generate() {
    try {
        output->setPlainText(buildReport());
    } catch (const utils::ClinicException& e) {
        QMessageBox::warning(this, "Report", QString::fromUtf8(e.what()));
    }
}

// Only the controls that matter for the chosen report are shown
void ReportPanel::onReportChanged() {
    int index = reportCombo->currentIndex();
    bool needsTop = (index == 0 || index == 4);
    bool needsDate = (index == 3 || index == 4);

    topLabel->setVisible(needsTop);
    topSpin->setVisible(needsTop);
    dateLabel->setVisible(needsDate);
    dateEdit->setVisible(needsDate);

    generate();
}

void ReportPanel::refresh() {
    generate();
}

void ReportPanel::exportReport() {
    QString path = QFileDialog::getSaveFileName(
        this, "Export report", "report.txt", "Text files (*.txt);;All files (*)");
    if (path.isEmpty()) return;       // the user pressed Cancel

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Export failed", "Cannot write to " + path);
        return;
    }

    QTextStream out(&file);
    out << output->toPlainText() << "\n";
    file.close();

    emit statusMessage("Report exported to " + path);
}

} // namespace gui
} // namespace scapms