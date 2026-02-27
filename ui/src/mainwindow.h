#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QListWidget>
#include <QComboBox>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QGroupBox>

// Added these so the compiler knows what your 'real implementation' is talking about
#include "graph_info.h"
#include "download_manager.h"
#include "snap_catalog.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() = default;

private slots:
    void onRunClicked();
    void onSyncClicked();
    void onLocalItemSelected(QListWidgetItem* item);
    void onSnapItemSelected(QListWidgetItem* item);

private:
    void setupManualLayout();
    void appendLog(const QString& message, bool isError = false);

    // REQUIRED: You added this in the .cpp, so it MUST be here too
    void loadSnapMetadata(const QString& path);

    // UI Components matching your stable layout
    QTabWidget* m_tabs = nullptr;
    QListWidget* m_listLocal = nullptr;
    QListWidget* m_listSnap = nullptr;
    QComboBox* m_comboAlgo = nullptr;
    QTextEdit* m_logArea = nullptr;

    QLabel* m_lblNodes = nullptr;
    QLabel* m_lblEdges = nullptr;
    QLabel* m_lblResult = nullptr;
    QLabel* m_lblStatus = nullptr;
    QPushButton* m_btnSync = nullptr;
    QProgressBar* m_progressBar = nullptr;

    QString m_selectedFilePath;
};

#endif