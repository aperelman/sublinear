#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>
#include <QFile>

class DownloadManager : public QObject {
    Q_OBJECT
    
public:
    explicit DownloadManager(QObject* parent = nullptr);
    
    void downloadFile(const QString& url, const QString& destinationPath);
    void cancel();
    
signals:
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished(const QString& filePath);
    void downloadError(const QString& errorMessage);
    
private slots:
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished();
    void onDownloadError(QNetworkReply::NetworkError error);
    
private:
    bool decompressGzip(const QString& gzipPath, const QString& outputPath);
    
    QNetworkAccessManager* networkManager;
    QNetworkReply* currentReply;
    QFile* outputFile;
    QString destinationPath;
};
