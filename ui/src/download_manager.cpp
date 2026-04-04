#include "download_manager.h"
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QFile>
#include <QDir>

DownloadManager::DownloadManager(QObject *parent) : QObject(parent) {
    manager = new QNetworkAccessManager(this);
}

void DownloadManager::validateUrl(const QUrl &url) {
    QNetworkRequest request(url);
    // HEAD request: Asks the server "Is it there?" without downloading the bytes.
    QNetworkReply *reply = manager->head(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, url]() {
        bool isValid = false;
        if (reply->error() == QNetworkReply::NoError) {
            int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (statusCode >= 200 && statusCode < 400) {
                isValid = true;
            }
        }
        Q_EMIT urlValidated(url, isValid);
        reply->deleteLater();
    });
}

void DownloadManager::startDownload(const QUrl &url, const QString &savePath) {
    QNetworkRequest request(url);
    QNetworkReply *reply = manager->get(request);
    reply->setProperty("savePath", savePath);
    reply->setProperty("isCatalog", false);
    m_activeReply = reply;

    connect(reply, &QNetworkReply::downloadProgress, this,
        [this](qint64 bytesReceived, qint64 bytesTotal) {
            Q_EMIT progress(bytesReceived, bytesTotal);
        });

    connect(reply, &QNetworkReply::finished, this, [this, reply]() { this->onFinished(reply); });
}

void DownloadManager::cancelDownload() {
    if (m_activeReply) {
        m_activeReply->abort();
    }
}

void DownloadManager::startCatalogDownload(const QUrl &url, const QString &savePath) {
    QNetworkRequest request(url);
    QNetworkReply *reply = manager->get(request);
    reply->setProperty("savePath", savePath);
    reply->setProperty("isCatalog", true);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { this->onFinished(reply); });
}

void DownloadManager::onFinished(QNetworkReply *reply) {
    QString savePath = reply->property("savePath").toString();
    bool isCatalog = reply->property("isCatalog").toBool();

    if (reply->error() == QNetworkReply::NoError) {
        QFile file(savePath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(reply->readAll());
            file.close();
            if (isCatalog) Q_EMIT catalogDownloaded(true);
            Q_EMIT finished(savePath);
        } else {
            if (isCatalog) Q_EMIT catalogDownloaded(false);
            Q_EMIT error("Could not open file for writing: " + savePath);
        }
    } else {
        // Don't emit error or finished on user-initiated abort
        if (reply->error() != QNetworkReply::OperationCanceledError) {
            if (isCatalog) Q_EMIT catalogDownloaded(false);
            Q_EMIT error("Download failed: " + reply->errorString());
        }
    }
    if (reply == m_activeReply) m_activeReply = nullptr;
    reply->deleteLater();
}