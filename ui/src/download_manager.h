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

    // Dataset download (.txt.gz)
    void startDownload(const QUrl &url, const QString &savePath);

    // SNAP index catalog download (.html)
    void startCatalogDownload(const QUrl &url, const QString &savePath);

    Q_SIGNALS: // Required by QT_NO_KEYWORDS in CMakeLists.txt
        void progress(int percentage);
    void finished(const QString &filePath);
    void error(const QString &message);
    void catalogDownloaded(bool success); // Signal for SnapBrowserWidget to parse index

private Q_SLOTS: // Required by QT_NO_KEYWORDS in CMakeLists.txt
    void onFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager *manager;
};

#endif