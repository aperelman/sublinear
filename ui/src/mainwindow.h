#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QTextEdit>
#include <QTabWidget>
#include "download_manager.h" // Include full definition for the owned manager

class SnapBrowserWidget;
class LocalFilesWidget; // Ensure this class is defined in your project


class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() = default;

public slots:
    void logMessage(const QString &msg);
    void updateProperties(const QString &name, int64_t nodes, int64_t edges, int64_t triangles);
    void handleRunClicked();

private:
    QTabWidget *tabWidget;
    SnapBrowserWidget *snapBrowser;
    LocalFilesWidget *localFilesTab;
    DownloadManager *downloadManager;

    QLabel *label_nodes;
    QLabel *label_edges;
    QLabel *label_triangles;
    QComboBox *algoSelection;
    QPushButton *btn_run;
    QTextEdit *messagePanel;

    QString m_currentSnapName;
};

#endif