#include "download_manager.h"
#include <QNetworkRequest>
#include <QUrl>
#include <QFileInfo>
#include <QDir>
#include <QProcess>

DownloadManager::DownloadManager(QObject* parent)
    : QObject(parent)
    , networkManager(new QNetworkAccessManager(this))
    , currentReply(nullptr)
    , outputFile(nullptr)
{
}

void DownloadManager::downloadFile(const QString& url, const QString& destPath) {
    destinationPath = destPath;
    
    QFileInfo fileInfo(destPath);
    QDir dir = fileInfo.dir();
    if (!dir.exists()) dir.mkpath(".");
    
    if (QFile::exists(destPath)) {
        emit downloadFinished(destPath);
        return;
    }
    
    QUrl qurl(url);
    QNetworkRequest request{qurl};  // Fix: use brace initialization
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, 
                        QNetworkRequest::NoLessSafeRedirectPolicy);
    
    currentReply = networkManager->get(request);
    
    connect(currentReply, &QNetworkReply::downloadProgress,
            this, &DownloadManager::onDownloadProgress);
    connect(currentReply, &QNetworkReply::finished,
            this, &DownloadManager::onDownloadFinished);
    connect(currentReply, &QNetworkReply::errorOccurred,
            this, &DownloadManager::onDownloadError);
    
    QString downloadPath = url.endsWith(".gz") ? destPath + ".gz" : destPath;
    outputFile = new QFile(downloadPath, this);
    if (!outputFile->open(QIODevice::WriteOnly)) {
        emit downloadError("Cannot open file");
        currentReply->abort();
    }
}

void DownloadManager::cancel() {
    if (currentReply) currentReply->abort();
}

void DownloadManager::onDownloadProgress(qint64 r, qint64 t) {
    emit downloadProgress(r, t);
}

void DownloadManager::onDownloadFinished() {
    if (!currentReply) return;
    
    if (outputFile) {
        outputFile->write(currentReply->readAll());
        outputFile->close();
        
        QString path = outputFile->fileName();
        delete outputFile;
        outputFile = nullptr;
        
        if (path.endsWith(".gz")) {
            QString out = path;
            out.chop(3);
            if (decompressGzip(path, out)) {
                emit downloadFinished(out);
            } else {
                emit downloadError("Decompress failed");
            }
        } else {
            emit downloadFinished(path);
        }
    }
    
    currentReply->deleteLater();
    currentReply = nullptr;
}

void DownloadManager::onDownloadError(QNetworkReply::NetworkError) {
    emit downloadError(currentReply ? currentReply->errorString() : "Error");
    
    if (outputFile) {
        outputFile->close();
        outputFile->remove();
        delete outputFile;
        outputFile = nullptr;
    }
    
    if (currentReply) {
        currentReply->deleteLater();
        currentReply = nullptr;
    }
}

bool DownloadManager::decompressGzip(const QString& gzip, const QString& out) {
    QProcess p;
    p.start("gunzip", QStringList() << "-c" << gzip);
    if (!p.waitForFinished(60000) || p.exitCode() != 0) return false;
    
    QFile f(out);
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(p.readAllStandardOutput());
    f.close();
    return true;
}
