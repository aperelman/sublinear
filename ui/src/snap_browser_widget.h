#pragma once

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QList>
#include <QString>
#include <QThread>
#include "graph_analyzer_worker.h"

// Forward declarations
struct GraphInfo;
class DownloadManager;
class QNetworkAccessManager;
class QNetworkReply;

class SnapBrowserWidget : public QWidget {
    Q_OBJECT
public:
    explicit SnapBrowserWidget(QWidget* parent = nullptr);
    ~SnapBrowserWidget();
    void setDatasets(const QList<GraphInfo>& datasets);
    void handleAnalysis(const QString& filePath);
    bool hasSelection() const;
    QString selectedFilePath() const;
    long long selectedTriangleCount() const;

signals:
    void datasetReady(const QString& filePath);
    void datasetSelected();
    void analysisProgress(const QString& message);

private slots:
    void onDatasetSelected();
    void onDownloadClicked();
    void onDownloadProgress(qint64 r, qint64 t);
    void onDownloadFinished(const QString& f);
    void onDownloadError(const QString& e);
    void onRefreshClicked();
    void onScrapeFinished(QNetworkReply* reply);
    void onDetailPageFinished(QNetworkReply* reply);
    void onAnalysisFinished(double result);
    void onAnalysisError(const QString& message);

private:
    void setupUI();
    void updateList();
    void loadFromCache();
    void saveToCache();
    void fetchTriangleCounts();
    QString getPath(const GraphInfo& ds);
    bool isDownloaded(const GraphInfo& ds);

    // UI
    QListWidget*  list       = nullptr;
    QLabel*       info       = nullptr;
    QPushButton*  btn        = nullptr;
    QPushButton*  refreshBtn = nullptr;
    QProgressBar* progress   = nullptr;

    // Logic
    QList<GraphInfo>       datasets;
    DownloadManager*       dlmgr          = nullptr;
    QNetworkAccessManager* networkManager = nullptr;
    QString                dlPath;
    
    // Worker
    QThread*             workerThread = nullptr;
    GraphAnalyzerWorker* worker       = nullptr;
};
