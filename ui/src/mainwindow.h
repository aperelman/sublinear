#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QJsonArray>
#include <QLabel>
#include <QTextEdit>
#include <QListWidget>
#include <QComboBox>
#include <QPushButton>
#include <QTabWidget>

class GraphListWidget; // Forward declaration

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void appendLog(const QString& message);

private slots:
    void onRefreshSnapRequested();
    void onDownloadClicked();
    void onRunClicked();
    void onSnapGraphSelected(const QString& name, const QString& urlPath);
    void onLocalItemSelected(const QString &name);

private:
    void setupManualLayout();
    void loadInitialData();
    void parseSubPageMetadata(const QString& html);

    // UI Elements
    QTabWidget *m_tabWidget;
    QListWidget *m_localList;
    GraphListWidget *m_graphList;

    QLabel *m_lblNodes;
    QLabel *m_lblEdges;
    QLabel *m_lblTriangles;

    QPushButton *m_btnRefreshSnap;
    QPushButton *m_btnDownload;
    QPushButton *m_runBtn;
    QComboBox *m_algoCombo;
    QTextEdit *m_logArea;

    // Data & Network
    QJsonArray m_cachedDatasets;
    QNetworkAccessManager *m_networkManager;
    QString m_currentSnapUrl;
};

#endif