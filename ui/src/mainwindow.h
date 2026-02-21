#pragma once

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QString>

class QTabWidget;
class QComboBox;
class QPushButton;
class QTextEdit;
class GraphListWidget;
class SnapBrowserWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() = default;

private slots:
    void checkRunRequirements();
    void onGraphSelected();
    void onGraphDoubleClicked();
    void onDatasetReady(const QString& filePath);
    void onRunAlgorithmClicked();
    void onAnalysisProgress(const QString& message);
    void handleNetworkReply();

private:
    void setupUI();
    void setupMenuBar();
    void loadSnapDatasets();
    void updateStatusBar(const QString& message);

    QTabWidget*          leftTabWidget   = nullptr;
    GraphListWidget*     graphListWidget  = nullptr;
    SnapBrowserWidget*   snapBrowserWidget = nullptr;
    QComboBox*           algorithmCombo  = nullptr;
    QPushButton*         runButton       = nullptr;
    QTextEdit*           resultsText     = nullptr;
    QNetworkAccessManager* networkManager = nullptr;

    QString currentGraphPath;
};