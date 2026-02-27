#include "download_manager.h"
#include <QNetworkRequest>
#include <QUrl>
#include <QFile>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>

DownloadManager::DownloadManager(QObject *parent) : QObject(parent) {
    m_manager = new QNetworkAccessManager(this);
}

void DownloadManager::startDownload(const QString& url, const QString& destinationPath) {
    QNetworkRequest request((QUrl(url)));
    request.setHeader(QNetworkRequest::UserAgentHeader, "GraphAnalyzer/1.0");

    QNetworkReply* reply = m_manager->get(request);

    connect(reply, &QNetworkReply::downloadProgress, this, &DownloadManager::downloadProgress);

    // Pass the destinationPath through the lambda to the handler
    connect(reply, &QNetworkReply::finished, this, [this, reply, destinationPath]() {
        onDownloadFinished(reply, destinationPath);
    });
}

void DownloadManager::onDownloadFinished(QNetworkReply* reply, const QString& destinationPath) {
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit downloadError(reply->errorString());
        emit finished("");
        return;
    }

    // Use provided path, otherwise use a default temp path
    QString finalPath = destinationPath;
    if (finalPath.isEmpty()) {
        finalPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/snap_resource.tmp";
    }

    // Ensure directory exists
    QFileInfo fileInfo(finalPath);
    QDir().mkpath(fileInfo.absolutePath());

    QFile file(finalPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(reply->readAll());
        file.close();

        emit downloadFinished(finalPath);
        emit finished(finalPath);
    } else {
        emit downloadError("Could not write to file: " + finalPath);
        emit finished("");
    }
}