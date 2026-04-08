#include "download_manager.h"
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QFile>
#include <QDir>
#include <QRegularExpression>

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
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    QNetworkReply *reply = manager->get(request);
    reply->setProperty("savePath", savePath);
    reply->setProperty("isCatalog", false);
    m_activeReply = reply;

    // Open file immediately and stream chunks as they arrive —
    // avoids buffering gigabytes in memory for large datasets
    auto *file = new QFile(savePath, reply);  // reply is parent, auto-deleted
    if (!file->open(QIODevice::WriteOnly)) {
        reply->abort();
        Q_EMIT error("Could not open file for writing: " + savePath);
        return;
    }

    connect(reply, &QNetworkReply::readyRead, this, [this, reply, file]() {
        if (file->write(reply->readAll()) == -1) {
            reply->abort();
            Q_EMIT error("Disk write failed: " + file->errorString());
        }
    });

    connect(reply, &QNetworkReply::downloadProgress, this,
        [this](qint64 bytesReceived, qint64 bytesTotal) {
            Q_EMIT progress(bytesReceived, bytesTotal);
        });

    connect(reply, &QNetworkReply::finished, this, [this, reply, file, savePath]() {
        file->flush();
        file->close();
        if (reply == m_activeReply) m_activeReply = nullptr;
        if (reply->error() == QNetworkReply::NoError) {
            Q_EMIT finished(savePath);
        } else if (reply->error() != QNetworkReply::OperationCanceledError) {
            QFile::remove(savePath);  // delete partial file on error
            Q_EMIT error("Download failed: " + reply->errorString());
        }
        reply->deleteLater();
    });
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
    // Only catalog downloads reach here now
    if (reply->error() == QNetworkReply::NoError) {
        QFile file(savePath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(reply->readAll());
            file.close();
            Q_EMIT catalogDownloaded(true);
        } else {
            Q_EMIT catalogDownloaded(false);
        }
    } else {
        if (reply->error() != QNetworkReply::OperationCanceledError)
            Q_EMIT catalogDownloaded(false);
    }
    reply->deleteLater();
}