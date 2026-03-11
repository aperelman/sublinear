#ifndef DOWNLOAD_MANAGER_H
#define DOWNLOAD_MANAGER_H

#include <QObject>
#include <QUrl>
#include <QFile>
#include <QNetworkAccessManager>

class DownloadManager : public QObject {
    Q_OBJECT
public:
    explicit DownloadManager(QObject *parent = nullptr);

    // Dataset download (.txt.gz)
    void startDownload(const QUrl &url, const QString &savePath);

    // SNAP index catalog download (.html)
    void startCatalogDownload(const QUrl &url, const QString &savePath);

    signals:
        void progress(int percentage);
    void finished(const QString &filePath);
    void error(const QString &message);

    // Emits true on successful save, false if download or write fails
    void catalogDownloaded(bool success);

private slots:
    void onFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager *manager;
};

#endif