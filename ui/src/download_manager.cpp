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
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { this->onFinished(reply); });
}

void DownloadManager::startCatalogDownload(const QUrl &url, const QString &savePath) {
    QNetworkRequest request(url);
    QNetworkReply *reply = manager->get(request);
    reply->setProperty("savePath", savePath);
    reply->setProperty("isCatalog", true); // Flag to identify list updates
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

            if (isCatalog) emit catalogDownloaded(true); // Signal successful write
            emit finished(savePath);
        } else {
            if (isCatalog) emit catalogDownloaded(false); // Signal write failure
            emit error("Could not open file for writing: " + savePath);
        }
    } else {
        if (isCatalog) emit catalogDownloaded(false); // Signal network failure
        emit error("Download failed: " + reply->errorString());
    }
    reply->deleteLater();
}