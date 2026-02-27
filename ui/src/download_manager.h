#ifndef DOWNLOAD_MANAGER_H
#define DOWNLOAD_MANAGER_H

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class DownloadManager : public QObject {
    Q_OBJECT
public:
    explicit DownloadManager(QObject *parent = nullptr);

    // Updated to accept optional destination path to match snap_browser_widget.cpp
    void startDownload(const QString& url, const QString& destinationPath = "");

    // Alias for snap_browser_widget.cpp
    void downloadFile(const QString& url, const QString& destinationPath = "") {
        startDownload(url, destinationPath);
    }

    signals:
        void finished(const QString& localPath);
    void downloadFinished(const QString& localPath);
    void downloadError(const QString& error);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);

private slots:
    void onDownloadFinished(QNetworkReply* reply, const QString& destinationPath);

private:
    QNetworkAccessManager* m_manager;
};

#endif