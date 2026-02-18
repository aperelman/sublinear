#pragma once

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QList>
#include <QString>

// Forward declarations to reduce header coupling
struct GraphInfo;
class DownloadManager;
class QNetworkAccessManager;
class QNetworkReply;

class SnapBrowserWidget : public QWidget {
    Q_OBJECT
public:
    explicit SnapBrowserWidget(QWidget* parent = nullptr); //
    void setDatasets(const QList<GraphInfo>& datasets); //

    /**
     * Public method to handle the transition from download to algorithm execution.
     * This is called by MainWindow when the "Run" button is clicked.
     */
    void handleAnalysis(const QString& filePath); 

signals:
    /**
     * Emitted when a dataset is downloaded and ready, or when a local file is selected.
     * Passing the filePath allows the MainWindow to track the active graph.
     */
    void datasetReady(const QString& filePath);

private slots:
    void onDatasetSelected(); //
    void onDownloadClicked(); //
    void onDownloadProgress(qint64 r, qint64 t); //
    void onDownloadFinished(const QString& f); //
    void onDownloadError(const QString& e); //
    void onRefreshClicked(); //
    void onScrapeFinished(QNetworkReply* reply); //
    void onDetailPageFinished(QNetworkReply* reply); //

private:
    void setupUI(); //
    void updateList(); //
    void loadFromCache(); //
    void saveToCache(); //
    void fetchTriangleCounts(); //
    QString getPath(const GraphInfo& ds); //
    bool isDownloaded(const GraphInfo& ds); //

    // UI Elements
    QListWidget* list = nullptr;
    QLabel* info = nullptr;
    QPushButton* btn = nullptr;
    QPushButton* refreshBtn = nullptr;
    QProgressBar* progress = nullptr;
    
    // Logic and Networking
    QList<GraphInfo> datasets;
    DownloadManager* dlmgr = nullptr;
    QNetworkAccessManager* networkManager = nullptr;
    QString dlPath;
};