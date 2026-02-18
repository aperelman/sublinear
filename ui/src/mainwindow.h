#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QTabWidget>
#include <QComboBox>
#include <QPushButton>
#include <QTextEdit>
#include <QString>

class GraphListWidget;
class SnapBrowserWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void handleNetworkReply();
    void onRunAlgorithmClicked();
    void onGraphSelected();
    void onGraphDoubleClicked();
    void onDatasetReady(const QString& filePath); // Updated signature
    void checkRunRequirements(); // New logic for button state

private:
    void setupUI();
    void setupMenuBar();
    void loadSnapDatasets();
    void updateStatusBar(const QString& message);

    // UI Pointers
    QTabWidget* leftTabWidget = nullptr;
    GraphListWidget* graphListWidget = nullptr;
    SnapBrowserWidget* snapBrowserWidget = nullptr;
    QComboBox* algorithmCombo = nullptr;
    QPushButton* runButton = nullptr;
    QTextEdit* resultsText = nullptr;
    QNetworkAccessManager* networkManager = nullptr;

    // State variable to track the active graph path
    QString currentGraphPath;
};

#endif // MAINWINDOW_H