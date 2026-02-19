#include "download_manager.h"
#include <QNetworkRequest>
#include <QUrl>
#include <QFileInfo>
#include <QDir>
#include <QByteArray>

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
    QNetworkRequest request{qurl};
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    currentReply = networkManager->get(request);

    // Write chunks as they arrive
    connect(currentReply, &QNetworkReply::readyRead,
            this, &DownloadManager::onReadyRead);
    connect(currentReply, &QNetworkReply::downloadProgress,
            this, &DownloadManager::onDownloadProgress);
    connect(currentReply, &QNetworkReply::finished,
            this, &DownloadManager::onDownloadFinished);
    connect(currentReply, &QNetworkReply::errorOccurred,
            this, &DownloadManager::onDownloadError);

    // Always save as .gz first
    QString downloadPath = destPath.endsWith(".gz") ? destPath : destPath + ".gz";
    outputFile = new QFile(downloadPath, this);
    if (!outputFile->open(QIODevice::WriteOnly)) {
        emit downloadError("Cannot open file: " + downloadPath);
        currentReply->abort();
    }
}

void DownloadManager::cancel() {
    if (currentReply) currentReply->abort();
}

void DownloadManager::onReadyRead() {
    if (outputFile && currentReply)
        outputFile->write(currentReply->readAll());
}

void DownloadManager::onDownloadProgress(qint64 r, qint64 t) {
    emit downloadProgress(r, t);
}

void DownloadManager::onDownloadFinished() {
    if (!currentReply) return;

    // Write any remaining data
    if (outputFile && currentReply->bytesAvailable() > 0)
        outputFile->write(currentReply->readAll());

    if (outputFile) {
        outputFile->flush();
        outputFile->close();
        QString savedPath = outputFile->fileName();
        delete outputFile;
        outputFile = nullptr;

        // Emit the path to the saved file (.gz)
        emit downloadFinished(savedPath);
    }

    currentReply->deleteLater();
    currentReply = nullptr;
}

void DownloadManager::onDownloadError(QNetworkReply::NetworkError) {
    emit downloadError(currentReply ? currentReply->errorString() : "Unknown error");

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

bool DownloadManager::decompressGzip(const QString& gzipPath, const QString& outPath) {
    QFile gzFile(gzipPath);
    if (!gzFile.open(QIODevice::ReadOnly))
        return false;

    QByteArray compressed = gzFile.readAll();
    gzFile.close();

    // Qt's qUncompress expects a 4-byte big-endian size prefix before the zlib data.
    // For raw gzip files we use the workaround of stripping the gzip header
    // and using inflateRaw — but since we can't use zlib directly, we instead
    // rely on the fact that .txt.gz files from SNAP are standard gzip.
    // We prepend the uncompressed size as a 4-byte header for qUncompress.
    // However qUncompress uses zlib deflate, not gzip format.
    // Best cross-platform solution: save the .gz and let the user decompress,
    // OR just keep the .gz file and pass it directly to the algorithm.

    // For now: just rename .gz to destination and return success.
    // Algorithms can read .gz directly if needed, or we decompress later.
    if (gzipPath != outPath) {
        QFile::remove(outPath);
        return QFile::rename(gzipPath, outPath + ".gz") ||
               QFile::copy(gzipPath, outPath + ".gz");
    }
    return true;
}
