#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <vector>

#include "controller/ClinicManager.h"
#include "gui/ClinicPanel.h"

class QAction;
class QLabel;
class QTabWidget;

namespace scapms {
namespace gui {

class SearchSortPanel;

class MainWindow : public QMainWindow {
    Q_OBJECT

private:
    controller::ClinicManager clinic;     // the ONE clinic shared by every panel
    QTabWidget* tabs;
    QLabel* summaryLabel;
    QAction* undoAction;
    SearchSortPanel* searchPanel;         // kept so Ctrl+F can jump to it
    std::vector<ClinicPanel*> panels;     // every tab, held through the base class
    QString currentFile;                  // empty until the user saves or loads

    void createMenus();
    void createStatusBar();
    void addPanel(ClinicPanel* panel, const QString& title);
    void saveTo(const QString& path);

private slots:
    void refreshAll();
    void showStatus(const QString& text);
    void newDatabase();
    void saveFile();
    void saveFileAs();
    void loadFile();
    void undoLast();
    void focusSearch();
    void showAbout();

public:
    explicit MainWindow(QWidget* parent = nullptr);
};

} // namespace gui
} // namespace scapms

#endif