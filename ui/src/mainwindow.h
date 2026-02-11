#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QTabWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QProgressBar>
#include <QTextEdit>
#include <QVector>

// Forward declarations: Tells the compiler these classes exist elsewhere
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
    void onDatasetReady();

private:
    void setupUI();
    void setupMenuBar();
    void loadSnapDatasets();
    void updateStatusBar(const QString& message);

    // UI Pointers - initialized to nullptr for safety
    QTabWidget* leftTabWidget = nullptr;
    GraphListWidget* graphListWidget = nullptr;
    SnapBrowserWidget* snapBrowserWidget = nullptr;
    QComboBox* algorithmCombo = nullptr;
    QSpinBox* maxKSpinBox = nullptr;
    QPushButton* runButton = nullptr;
    QProgressBar* progressBar = nullptr;
    QTabWidget* tabWidget = nullptr;
    QTextEdit* resultsText = nullptr;

    QNetworkAccessManager* networkManager = nullptr;
};

#endif // MAINWINDOW_H