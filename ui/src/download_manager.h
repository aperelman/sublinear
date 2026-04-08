#ifndef DOWNLOAD_MANAGER_H
#define DOWNLOAD_MANAGER_H

#include <QObject>
#include <QUrl>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class DownloadManager : public QObject {
    Q_OBJECT
public:
    explicit DownloadManager(QObject *parent = nullptr);

    void startDownload(const QUrl &url, const QString &savePath);
    void startCatalogDownload(const QUrl &url, const QString &savePath);
    void validateUrl(const QUrl &url);

    // Fetch dataset HTML page and resolve the real .txt.gz download URL

    void cancelDownload();
    bool isDownloading() const { return m_activeReply != nullptr; }

Q_SIGNALS:
    void progress(qint64 bytesReceived, qint64 bytesTotal);
    void finished(const QString &filePath);
    void error(const QString &message);
    void catalogDownloaded(bool success);
    void urlValidated(const QUrl &url, bool isValid);

private Q_SLOTS:
    void onFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager *manager;
    QNetworkReply *m_activeReply = nullptr;
};

#endif
