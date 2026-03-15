#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QTextEdit>
#include <QLabel>
#include <QUrl>
#include <memory>
#include "algorithm_runner.h"
#include "snap_browser_widget.h"
#include "download_manager.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private Q_SLOTS:
    void handleRunClicked();
    void updateProperties(const QString& name, int64_t nodes, int64_t edges, int64_t triangles);
    void onAlgoSelectionChanged(const QString &algo);
    void handleLogMessage(const QString &message);

private:
    void runAlgorithmOnFile(const QString &filePath);
    void updateStatusLabel();
    QString localPathForDataset(const QString &name) const;
    static bool decompressGz(const QString &gzPath, const QString &outPath);

    // Colored log helpers
    void logPhase(const QString &msg);    // phase headers  — dark blue
    void logInfo(const QString &msg);     // normal info    — black
    void logGraph(const QString &msg);    // graph load info — purple
    void logResult(const QString &msg);   // results        — dark green
    void logWarning(const QString &msg);  // warnings       — orange
    void logError(const QString &msg);    // errors         — red
    void logHtml(const QString &html);    // raw HTML

    // Report
    void appendReport(const QString &graphName, const QString &algo,
                      int64_t nodes, int64_t edges,
                      int64_t exactTriangles, int64_t estimatedTriangles,
                      double arboricity, double elapsedMs);
    static QString reportsFilePath();

    QLineEdit   *m_editFilePath;
    QComboBox   *m_algoSelection;
    QLabel      *m_degeneracyLabel;
    QSpinBox    *m_degeneracySpinBox;
    QLabel      *m_labelStatus;
    QPushButton *m_btnRun;
    QLabel      *m_labelNodes;
    QLabel      *m_labelEdges;
    QLabel      *m_labelTriangles;
    QTextEdit   *m_textLog;

    QString  m_pendingSnapName;
    QUrl     m_pendingSnapUrl;
    int64_t  m_pendingSnapTriangles  = 0;
    double   m_pendingSnapArboricity = 0.0;

    // Pipeline state for 3-step auto flow
    QString  m_pipelineFilePath;
    bool     m_pipelineNeedTriangles = false;

    // Run tracking for report
    qint64   m_runStartMs = 0;
    int64_t  m_lastExactTriangles = 0;
    int64_t  m_lastEstimatedTriangles = 0;

    SnapBrowserWidget                *m_snapBrowser;
    std::unique_ptr<DownloadManager>  m_downloadManager;
    std::unique_ptr<AlgorithmRunner>  m_runner;
};

#endif
