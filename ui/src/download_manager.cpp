#include "download_manager.h"
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QFile>
#include <QDir>

DownloadManager::DownloadManager(QObject *parent) : QObject(parent) {
    manager = new QNetworkAccessManager(this);
}

void DownloadManager::startDownload(const QUrl &url, const QString &savePath) {
    QNetworkRequest request(url);
    QNetworkReply *reply = manager->get(request);
    reply->setProperty("savePath", savePath);
    reply->setProperty("isCatalog", false);

    // Modern syntax compatible with AUTOMOC
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { this->onFinished(reply); });
}

void DownloadManager::startCatalogDownload(const QUrl &url, const QString &savePath) {
    QNetworkRequest request(url);
    QNetworkReply *reply = manager->get(request);
    reply->setProperty("savePath", savePath);
    reply->setProperty("isCatalog", true); // Flag to identify index updates

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

            if (isCatalog) {
                Q_EMIT catalogDownloaded(true); // Signal successful write for parser
            }
            Q_EMIT finished(savePath);
        } else {
            if (isCatalog) Q_EMIT catalogDownloaded(false);
            Q_EMIT error("Could not open file for writing: " + savePath);
        }
    } else {
        if (isCatalog) Q_EMIT catalogDownloaded(false);
        Q_EMIT error("Download failed: " + reply->errorString());
    }
    reply->deleteLater();
}