#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QTextEdit>
#include <QTextBrowser>
#include <QLabel>
#include <QUrl>
#include <QCloseEvent>
#include <memory>
#include <functional>
#include <QSplitter>
#include <QTabWidget>
#include <QTreeView>
#include <QFileSystemModel>
#include <QDockWidget>
#include <QListWidget>

#include "algorithm_runner.h"
#include "snap_browser_widget.h"
#include "download_manager.h"
#include "snap_dataset_cache.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private Q_SLOTS:
    void handleRunClicked();
    void handleDownloadFinished(const QString &gzPath);
    void handleCancelDownload();
    void handleClearCache();
    void updateProperties(const QString& name, int64_t nodes, int64_t edges, int64_t triangles);
    void onAlgoSelectionChanged(const QString &algo);
    void onArbMethodChanged(const QString &method);
    void handleLogMessage(const QString &message);
    void handleArboricityFinished(double arboricity);
    void onLocalFileSelected(const QModelIndex &index);

private:
    void startAnalysis(const QString &filePath);
    void setRunButton(bool running);
    ArboricityMethod currentArbMethod() const;
    double           currentManualArboricity() const;
    bool decompressGz(const QString &gzPath, const QString &outPath,
                      std::function<void(qint64)> progressCb = nullptr);
    QString localPathForDataset(const QString &name) const;
    QString statsCachePath() const;
    QString reportsFolderPath() const;

    // Logging Helpers
    void logHtml(const QString &html);
    void logError(const QString &msg);

    // Report viewer
    void setupReportDock();
    void refreshReportList();
    void saveReport();

    // UI Components
    QSplitter        *m_splitter;
    QTabWidget       *m_tabWidget;
    SnapBrowserWidget *m_snapBrowser;
    QTreeView        *m_localFileView;
    QFileSystemModel *m_fileModel;
    QLineEdit        *m_localFolderEdit;

    // Right panel controls
    QLineEdit   *m_editFilePath;
    QComboBox   *m_algoSelection;
    QLabel      *m_degeneracyLabel;
    QSpinBox    *m_degeneracySpinBox;
    QComboBox   *m_arbMethodSelection  = nullptr;
    QWidget     *m_arbMethodRow        = nullptr;
    QDoubleSpinBox *m_arbManualSpinBox = nullptr;
    QLabel      *m_arbManualLabel      = nullptr;
    QLabel      *m_labelStatus;
    QPushButton *m_btnRun;
    QPushButton *m_btnCancelDownload = nullptr;
    QPushButton *m_btnClearCache     = nullptr;
    QLabel      *m_labelNodes;
    QLabel      *m_labelEdges;
    QLabel      *m_labelTriangles;
    QTextEdit    *m_textLog;

    // Report viewer dock
    QDockWidget  *m_reportDock    = nullptr;
    QListWidget  *m_reportList    = nullptr;
    QTextBrowser *m_reportBrowser = nullptr;

    // State
    QString  m_pendingSnapName;
    QUrl     m_pendingSnapUrl;
    int64_t  m_pendingSnapTriangles = 0;
    int64_t  m_exactTriangleCount = 0;
    double   m_pendingSnapArboricity = 0.0;
    bool     m_isAnalysisRunning = false;
    bool     m_pendingChainedImportanceSampling = false;
    QString  m_chainedFilePath;
    QString  m_lastAnalyzedPath;
    QString  m_pendingGzPath;

    std::unique_ptr<DownloadManager> m_downloadManager;
    SnapDatasetCache m_statsCache;
    std::unique_ptr<AlgorithmRunner> m_runner;
};

#endif
