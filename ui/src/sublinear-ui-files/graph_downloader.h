#ifndef GRAPH_DOWNLOADER_H
#define GRAPH_DOWNLOADER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>
#include <QFile>
#include "graph_info.h"

class GraphDownloader : public QObject {
    Q_OBJECT

public:
    explicit GraphDownloader(QObject *parent = nullptr);
    ~GraphDownloader();

    void downloadGraph(const GraphInfo& graph);
    void cancelDownload();
    bool isDownloading() const { return currentReply != nullptr; }

signals:
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished(bool success, const QString& errorMessage);
    void extractionProgress(int percentage);
    void extractionFinished(bool success, const QString& errorMessage);

private slots:
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished();
    void onDownloadError(QNetworkReply::NetworkError error);

private:
    void extractGzFile(const QString& gzFilePath, const QString& outputPath);
    
    QNetworkAccessManager* networkManager;
    QNetworkReply* currentReply;
    QFile* downloadFile;
    GraphInfo currentGraph;
};

#endif // GRAPH_DOWNLOADER_H
