#include "gui/SearchSortPanel.h"

#include <QBrush>
#include <QComboBox>
#include <QFont>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include "controller/ClinicManager.h"
#include "gui/PatientStyle.h"
#include "utils/ClinicException.h"
#include "utils/StringUtils.h"

namespace scapms {
namespace gui {

using model::PatientRecord;

// ============================ construction ============================

SearchSortPanel::SearchSortPanel(controller::ClinicManager& clinic, QWidget* parent)
    : ClinicPanel(clinic, parent), searching(false) {

    // ---- The table (read-only; sorting and searching change what it shows) ----
    tableTitle = new QLabel();
    QFont titleFont = tableTitle->font();
    titleFont.setBold(true);
    tableTitle->setFont(titleFont);

    table = new QTableWidget(0, 6);
    QStringList headers;
    headers << "ID" << "Name" << "Age" << "Contact" << "Category" << "Priority";
    table->setHorizontalHeaderLabels(headers);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setStretchLastSection(true);

    QVBoxLayout* leftSide = new QVBoxLayout();
    leftSide->addWidget(tableTitle);
    leftSide->addWidget(table);

    QVBoxLayout* rightSide = new QVBoxLayout();
    rightSide->addWidget(buildSortBox());
    rightSide->addWidget(buildSearchBox());
    rightSide->addStretch();

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->addLayout(leftSide, 5);
    layout->addLayout(rightSide, 3);

    fillMethodCombo();

    // ---- Events ----
    connect(sortButton, &QPushButton::clicked, this, &SearchSortPanel::sortPatients);
    connect(sortAlgorithmCombo, &QComboBox::currentIndexChanged, this, &SearchSortPanel::updateSortExplanation);

    connect(searchTypeCombo, &QComboBox::currentIndexChanged, this, &SearchSortPanel::onSearchTypeChanged);
    connect(searchMethodCombo, &QComboBox::currentIndexChanged, this, &SearchSortPanel::onSearchTextChanged);
    connect(searchEdit, &QLineEdit::textChanged, this, &SearchSortPanel::onSearchTextChanged);   // live search
    connect(searchEdit, &QLineEdit::returnPressed, this, &SearchSortPanel::searchClicked);
    connect(searchButton, &QPushButton::clicked, this, &SearchSortPanel::searchClicked);
    connect(showAllButton, &QPushButton::clicked, this, &SearchSortPanel::showAll);

    refresh();
}

QWidget* SearchSortPanel::buildSortBox() {
    sortAlgorithmCombo = new QComboBox();
    sortAlgorithmCombo->addItem("Insertion Sort");
    sortAlgorithmCombo->addItem("Merge Sort");
    sortAlgorithmCombo->setToolTip("Which sorting algorithm to use");

    sortFieldCombo = new QComboBox();
    sortFieldCombo->addItem("Patient name");
    sortFieldCombo->addItem("Patient ID");
    sortFieldCombo->addItem("Age");
    sortFieldCombo->setToolTip("What to sort the patients by");

    sortButton = new QPushButton("SORT");
    sortButton->setToolTip("Sort the patient list with the chosen algorithm");

    sortExplanation = new QLabel();
    sortExplanation->setWordWrap(true);
    sortStatus = new QLabel();
    sortStatus->setWordWrap(true);

    QGroupBox* box = new QGroupBox("Sort");
    QFormLayout* form = new QFormLayout(box);
    form->addRow("Sort algorithm:", sortAlgorithmCombo);
    form->addRow("Sort by:", sortFieldCombo);
    form->addRow(sortButton);
    form->addRow(sortExplanation);
    form->addRow(sortStatus);
    return box;
}

QWidget* SearchSortPanel::buildSearchBox() {
    searchTypeCombo = new QComboBox();
    searchTypeCombo->addItem("Patient ID");
    searchTypeCombo->addItem("Patient name");
    searchTypeCombo->addItem("Category");
    searchTypeCombo->setToolTip("What to search for");

    searchMethodCombo = new QComboBox();
    searchMethodCombo->setToolTip("Which searching technique to use");

    searchEdit = new QLineEdit();
    searchEdit->setToolTip("The results update as you type (Ctrl+F jumps here)");

    searchButton = new QPushButton("Search");
    searchButton->setToolTip("Search now (a problem is explained in a dialog)");
    showAllButton = new QPushButton("Show all");
    showAllButton->setToolTip("Clear the search and show every patient");

    searchStatus = new QLabel();
    searchStatus->setWordWrap(true);

    QHBoxLayout* buttons = new QHBoxLayout();
    buttons->addWidget(searchButton);
    buttons->addWidget(showAllButton);

    QGroupBox* box = new QGroupBox("Search");
    QFormLayout* form = new QFormLayout(box);
    form->addRow("Search by:", searchTypeCombo);
    form->addRow("Method:", searchMethodCombo);
    form->addRow("Keyword:", searchEdit);
    form->addRow(buttons);
    form->addRow(searchStatus);
    return box;
}

// The methods on offer depend on what is being searched for
void SearchSortPanel::fillMethodCombo() {
    QSignalBlocker blocker(searchMethodCombo);    // do not run a search while refilling
    searchMethodCombo->clear();

    int type = searchTypeCombo->currentIndex();
    if (type == 0) {            // ID
        searchMethodCombo->addItem("Automatic (best for the data)",
                                   static_cast<int>(controller::SearchMethod::Automatic));
        searchMethodCombo->addItem("Linear search",
                                   static_cast<int>(controller::SearchMethod::Linear));
        searchMethodCombo->addItem("Binary search (list must be sorted by ID)",
                                   static_cast<int>(controller::SearchMethod::Binary));
        searchEdit->setPlaceholderText("Patient ID, e.g. 3");
    } else if (type == 1) {     // name
        searchMethodCombo->addItem("Recursive search (name contains...)",
                                   static_cast<int>(controller::SearchMethod::Recursive));
        searchMethodCombo->addItem("Binary search (exact name, sorted by name)",
                                   static_cast<int>(controller::SearchMethod::Binary));
        searchEdit->setPlaceholderText("Part of a name, e.g. bello");
    } else {                    // category
        searchMethodCombo->addItem("Linear scan",
                                   static_cast<int>(controller::SearchMethod::Linear));
        searchEdit->setPlaceholderText("General, Emergency or Follow-Up");
    }
    searchMethodCombo->setEnabled(type != 2);
}

// ============================ the table ============================

void SearchSortPanel::showPatients(const std::vector<PatientRecord*>& patients) {
    table->setRowCount(static_cast<int>(patients.size()));

    for (size_t i = 0; i < patients.size(); i++) {
        const PatientRecord& patient = *patients[i];
        int row = static_cast<int>(i);
        QColor tint = priorityColor(patient.getPriority());

        QStringList cells;
        cells << QString::number(patient.getId())
              << QString::fromStdString(patient.getName())
              << QString::number(patient.getAge())
              << QString::fromStdString(patient.getContact())
              << QString::fromStdString(patient.getCategory())
              << priorityLabel(patient.getPriority());

        for (int col = 0; col < cells.size(); col++) {
            QTableWidgetItem* item = new QTableWidgetItem(cells[col]);
            item->setBackground(QBrush(tint));
            table->setItem(row, col, item);
        }
    }
    table->resizeColumnsToContents();

    QString what = searching ? QString("Search results") : QString("All patients (current order)");
    tableTitle->setText(QString("%1: %2").arg(what).arg(static_cast<int>(patients.size())));
}

void SearchSortPanel::refresh() {
    if (searching) runSearch(false);              // keep the search results up to date
    else showPatients(clinic.getPatients());

    sortStatus->setText("Current order: " + QString::fromStdString(clinic.getSortStatus()));
    updateSortExplanation();
}

void SearchSortPanel::focusSearch() {
    searchEdit->setFocus();
    searchEdit->selectAll();
}

// ============================ sorting ============================

void SearchSortPanel::updateSortExplanation() {
    int n = static_cast<int>(clinic.patientCount());
    QString text;

    if (sortAlgorithmCombo->currentIndex() == 0) {
        text = "Insertion sort slides each patient into place, like sorting playing cards. "
               "It is very fast on small or nearly sorted lists (about n comparisons) "
               "but slows down on large, shuffled lists (about n x n).";
    } else {
        text = "Merge sort splits the list in half, sorts each half and merges them. "
               "It always needs about n x log n comparisons, so it stays fast on large "
               "or unpredictable lists. The price is some extra memory.";
    }

    if (n <= 20) {
        text += QString("\n\nWith %1 patient(s) the list is small: either algorithm works, "
                        "and insertion sort is the simpler choice.").arg(n);
    } else {
        text += QString("\n\nWith %1 patients the list is large: merge sort is the safer choice.").arg(n);
    }
    sortExplanation->setText(text);
}

void SearchSortPanel::sortPatients() {
    controller::SortAlgorithm algorithm = (sortAlgorithmCombo->currentIndex() == 0)
        ? controller::SortAlgorithm::Insertion
        : controller::SortAlgorithm::Merge;

    controller::SortField field = controller::SortField::Name;
    if (sortFieldCombo->currentIndex() == 1) field = controller::SortField::Id;
    else if (sortFieldCombo->currentIndex() == 2) field = controller::SortField::Age;

    try {
        clinic.sortPatients(algorithm, field);
        int comparisons = clinic.getLastSortComparisons();
        QString order = QString::fromStdString(clinic.getSortStatus());

        emit dataChanged();      // every tab now shows the new order

        sortStatus->setText(QString("Current order: %1\n%2 made %3 comparison(s).")
                                .arg(order)
                                .arg(sortAlgorithmCombo->currentText())
                                .arg(comparisons));
        emit statusMessage(QString("%1: %2 using %3 (%4 comparisons).")
                               .arg(order)
                               .arg(static_cast<int>(clinic.patientCount()))
                               .arg(sortAlgorithmCombo->currentText())
                               .arg(comparisons));
    } catch (const utils::ClinicException& e) {
        QMessageBox::warning(this, "Cannot sort", QString::fromUtf8(e.what()));
    }
}

// ============================ searching ============================

void SearchSortPanel::onSearchTypeChanged() {
    fillMethodCombo();
    runSearch(false);
}

void SearchSortPanel::onSearchTextChanged() {
    runSearch(false);      // live search: problems appear in the status line, not as dialogs
}

void SearchSortPanel::searchClicked() {
    if (searchEdit->text().trimmed().isEmpty()) {
        QMessageBox::information(this, "Search", "Type a patient ID, a name or a category first.");
        return;
    }
    runSearch(true);       // an explicit search explains problems in a dialog
}

void SearchSortPanel::showAll() {
    searchEdit->clear();
    searchEdit->setFocus();
    runSearch(false);
}

void SearchSortPanel::runSearch(bool showDialogs) {
    QString text = searchEdit->text().trimmed();

    if (text.isEmpty()) {                      // nothing typed: show everyone again
        searching = false;
        showPatients(clinic.getPatients());
        searchStatus->setText("Type to search. The table shows every patient.");
        return;
    }

    try {
        std::vector<PatientRecord*> results;
        int type = searchTypeCombo->currentIndex();
        controller::SearchMethod method = static_cast<controller::SearchMethod>(
            searchMethodCombo->currentData().toInt());

        if (type == 0) {                       // by ID
            int id = 0;
            if (!utils::tryParseInt(text.toStdString(), id)) {
                throw utils::InvalidDataException("Patient ID must be a whole number.");
            }
            PatientRecord* found = clinic.searchById(id, method);
            if (found != nullptr) results.push_back(found);
        } else if (type == 1) {                // by name
            results = clinic.searchByName(text.toStdString(), method);
        } else {                               // by category
            results = clinic.searchByCategory(text.toStdString());
        }

        searching = true;
        showPatients(results);
        searchStatus->setText(QString("%1 match(es) | %2 | %3 comparison(s)")
                                  .arg(static_cast<int>(results.size()))
                                  .arg(QString::fromStdString(clinic.getLastSearchMethod()))
                                  .arg(clinic.getLastSearchComparisons()));
    } catch (const utils::ClinicException& e) {
        QString message = QString::fromUtf8(e.what());

        searching = false;                     // fall back to the full list
        showPatients(clinic.getPatients());
        searchStatus->setText("Cannot search: " + message);

        if (showDialogs) {
            QMessageBox::warning(this, "Search", message);
        }
    }
}

} // namespace gui
} // namespace scapms