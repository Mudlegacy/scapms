#ifndef SEARCHSORTPANEL_H
#define SEARCHSORTPANEL_H

#include <vector>

#include "gui/ClinicPanel.h"
#include "model/PatientRecord.h"

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;

namespace scapms {
namespace gui {

class SearchSortPanel : public ClinicPanel {
    Q_OBJECT

private:
    QTableWidget* table;
    QLabel* tableTitle;

    // Sort controls
    QComboBox* sortAlgorithmCombo;
    QComboBox* sortFieldCombo;
    QPushButton* sortButton;
    QLabel* sortExplanation;
    QLabel* sortStatus;

    // Search controls
    QComboBox* searchTypeCombo;
    QComboBox* searchMethodCombo;
    QLineEdit* searchEdit;
    QPushButton* searchButton;
    QPushButton* showAllButton;
    QLabel* searchStatus;

    bool searching;        // true while the table shows search results, not the whole list

    QWidget* buildSortBox();
    QWidget* buildSearchBox();
    void fillMethodCombo();
    void showPatients(const std::vector<model::PatientRecord*>& patients);
    void runSearch(bool showDialogs);

private slots:
    void sortPatients();
    void updateSortExplanation();
    void onSearchTypeChanged();
    void onSearchTextChanged();
    void searchClicked();
    void showAll();

public:
    explicit SearchSortPanel(controller::ClinicManager& clinic, QWidget* parent = nullptr);
    void refresh() override;
    void focusSearch();    // used by Ctrl+F
};

} // namespace gui
} // namespace scapms

#endif