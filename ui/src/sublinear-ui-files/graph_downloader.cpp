#include "graph_downloader.h"
#include <QNetworkRequest>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <zlib.h>

GraphDownloader::GraphDownloader(QObject *parent)
    : QObject(parent)
    , networkManager(new QNetworkAccessManager(this))
    , currentReply(nullptr)
    , downloadFile(nullptr)
{
}

GraphDownloader::~GraphDownloader() {
    cancelDownload();
}

void GraphDownloader::downloadGraph(const GraphInfo& graph) {
    if (isDownloading()) {
        qWarning() << "Download already in progress";
        return;
    }

    currentGraph = graph;
    
    // Create graphs directory if it doesn't exist
    QDir dir;
    QString graphDir = QString::fromStdString(getGraphDirectory());
    if (!dir.exists(graphDir)) {
        dir.mkpath(graphDir);
    }

    // Prepare download file path
    QString downloadPath = graphDir + "/" + QString::fromStdString(graph.filename);
    downloadFile = new QFile(downloadPath + ".tmp");
    
    if (!downloadFile->open(QIODevice::WriteOnly)) {
        emit downloadFinished(false, "Cannot open file for writing: " + downloadPath);
        delete downloadFile;
        downloadFile = nullptr;
        return;
    }

    // Start download
    QNetworkRequest request(QUrl(QString::fromStdString(graph.url)));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, 
                        QNetworkRequest::NoLessSafeRedirectPolicy);
    
    currentReply = networkManager->get(request);
    
    connect(currentReply, &QNetworkReply::downloadProgress,
            this, &GraphDownloader::onDownloadProgress);
    connect(currentReply, &QNetworkReply::finished,
            this, &GraphDownloader::onDownloadFinished);
    connect(currentReply, &QNetworkReply::errorOccurred,
            this, &GraphDownloader::onDownloadError);
    connect(currentReply, &QIODevice::readyRead, [this]() {
        if (downloadFile) {
            downloadFile->write(currentReply->readAll());
        }
    });
}

void GraphDownloader::cancelDownload() {
    if (currentReply) {
        currentReply->abort();
        currentReply->deleteLater();
        currentReply = nullptr;
    }
    
    if (downloadFile) {
        downloadFile->close();
        downloadFile->remove();
        delete downloadFile;
        downloadFile = nullptr;
    }
}

void GraphDownloader::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    emit downloadProgress(bytesReceived, bytesTotal);
}

void GraphDownloader::onDownloadFinished() {
    if (!currentReply) return;

    bool success = (currentReply->error() == QNetworkReply::NoError);
    QString errorMessage;

    if (success) {
        // Write any remaining data
        if (downloadFile) {
            downloadFile->write(currentReply->readAll());
            downloadFile->close();
            
            // Rename from .tmp to actual filename
            QString graphDir = QString::fromStdString(getGraphDirectory());
            QString finalPath = graphDir + "/" + QString::fromStdString(currentGraph.filename);
            QString tempPath = finalPath + ".tmp";
            
            QFile::remove(finalPath);
            if (QFile::rename(tempPath, finalPath)) {
                // If it's a .gz file, extract it
                if (finalPath.endsWith(".gz")) {
                    QString extractedPath = finalPath;
                    extractedPath.chop(3); // Remove .gz extension
                    extractGzFile(finalPath, extractedPath);
                } else {
                    emit downloadFinished(true, "");
                }
            } else {
                errorMessage = "Failed to rename downloaded file";
                success = false;
            }
        }
    } else {
        errorMessage = currentReply->errorString();
    }

    // Cleanup
    if (downloadFile) {
        delete downloadFile;
        downloadFile = nullptr;
    }
    
    currentReply->deleteLater();
    currentReply = nullptr;

    if (!success || !errorMessage.isEmpty()) {
        emit downloadFinished(false, errorMessage);
    }
}

void GraphDownloader::onDownloadError(QNetworkReply::NetworkError error) {
    Q_UNUSED(error);
}

void GraphDownloader::extractGzFile(const QString& gzFilePath, const QString& outputPath) {
    emit extractionProgress(0);
    
    gzFile gz = gzopen(gzFilePath.toUtf8().constData(), "rb");
    if (!gz) {
        emit extractionFinished(false, "Failed to open .gz file");
        return;
    }

    QFile outFile(outputPath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        gzclose(gz);
        emit extractionFinished(false, "Failed to create output file");
        return;
    }

    const int bufferSize = 128 * 1024;
    char buffer[bufferSize];
    int bytesRead;
    qint64 totalRead = 0;
    QFileInfo gzInfo(gzFilePath);
    qint64 gzSize = gzInfo.size();

    while ((bytesRead = gzread(gz, buffer, bufferSize)) > 0) {
        outFile.write(buffer, bytesRead);
        totalRead += bytesRead;
        
        int progress = qMin(99, (int)((totalRead / 3) * 100 / gzSize));
        emit extractionProgress(progress);
    }

    outFile.close();
    gzclose(gz);

    if (bytesRead < 0) {
        QFile::remove(outputPath);
        emit extractionFinished(false, "Error during decompression");
    } else {
        QFile::remove(gzFilePath);
        emit extractionProgress(100);
        emit extractionFinished(true, "");
        emit downloadFinished(true, "");
    }
}
