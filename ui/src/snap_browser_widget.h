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
    explicit SnapBrowserWidget(QWidget* parent = nullptr);
    void setDatasets(const QList<GraphInfo>& datasets);
    void handleAnalysis(const QString& filePath);
    bool hasSelection() const;        // true if any dataset is selected in the list
    QString selectedFilePath() const; // returns local path if downloaded, empty otherwise

signals:
    void datasetReady(const QString& filePath);  // emitted when download completes or local file activated
    void datasetSelected();                       // emitted on any list selection change

private slots:
    void onDatasetSelected();
    void onDownloadClicked();
    void onDownloadProgress(qint64 r, qint64 t);
    void onDownloadFinished(const QString& f);
    void onDownloadError(const QString& e);
    void onRefreshClicked();
    void onScrapeFinished(QNetworkReply* reply);
    void onDetailPageFinished(QNetworkReply* reply);

private:
    void setupUI();
    void updateList();
    void loadFromCache();
    void saveToCache();
    void fetchTriangleCounts();
    QString getPath(const GraphInfo& ds);
    bool isDownloaded(const GraphInfo& ds);

    // UI Elements
    QListWidget*  list       = nullptr;
    QLabel*       info       = nullptr;
    QPushButton*  btn        = nullptr;
    QPushButton*  refreshBtn = nullptr;
    QProgressBar* progress   = nullptr;

    // Logic and Networking
    QList<GraphInfo>       datasets;
    DownloadManager*       dlmgr          = nullptr;
    QNetworkAccessManager* networkManager = nullptr;
    QString                dlPath;
};
