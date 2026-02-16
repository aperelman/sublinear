#include "snap_dataset_cache.h"
#include <QFile>
#include <QSaveFile>
#include <QDir>
#include <QStandardPaths>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFileInfo>
#include <QDateTime>
#include <ranges>       // C++23 Ranges
#include <algorithm>    // std::ranges::copy
#include"graph_info.h"

QString SnapDatasetCache::getCachePath() {
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir(cacheDir).mkpath(".");
    return QDir(cacheDir).filePath("snap_datasets.json");
}

bool SnapDatasetCache::cacheExists() {
    return QFile::exists(getCachePath());
}

QDateTime SnapDatasetCache::getCacheTimestamp() {
    return QFileInfo(getCachePath()).lastModified();
}

// --- SAVE LOGIC ---

bool SnapDatasetCache::saveToCache(const QList<GraphInfo>& datasets) {
    QSaveFile file(getCachePath());
    
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    // Explicitly call the helper that returns a QJsonDocument
    QJsonDocument doc = datasetsToDocument(datasets);
    file.write(doc.toJson());

    // commit() performs an atomic rename (swap) for safety
    return file.commit(); 
}

QJsonDocument SnapDatasetCache::datasetsToDocument(const QList<GraphInfo>& datasets) {
    QJsonObject root;
    root["datasets"] = datasetsToSnapshot(datasets);
    root["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    root["version"] = "1.0";
    
    return QJsonDocument(root);
}

QJsonArray SnapDatasetCache::datasetsToSnapshot(const QList<GraphInfo>& datasets) {
    QJsonArray array;
    for (const auto& ds : datasets) {
        array.append(QJsonObject{
            {"name", ds.name},
            {"description", ds.description},
            {"url", ds.url},
            {"filename", ds.filename},
            {"category", ds.category},
            {"nodes", (long long)ds.numNodes},
            {"edges", (long long)ds.numEdges},
            {"triangles", (long long)ds.numTriangles}
        });
    }
    return array;
}

// --- LOAD LOGIC ---

QList<GraphInfo> SnapDatasetCache::loadFromCache() {
    QFile file(getCachePath());
    if (!file.open(QIODevice::ReadOnly)) {
        return loadBuiltInSnapshot();
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    // This calls the QJsonDocument overload of jsonToDatasets
    return jsonToDatasets(QJsonDocument::fromJson(data));
}

// Overload 1: Takes QJsonDocument (The "Envelope")
QList<GraphInfo> SnapDatasetCache::jsonToDatasets(const QJsonDocument& doc) {
    if (doc.isNull()) return {};
    
    // Extract the array and delegate to the worker overload
    return jsonToDatasets(doc.object()["datasets"].toArray());
}

// Overload 2: Takes QJsonArray (The "Letter")
QList<GraphInfo> SnapDatasetCache::jsonToDatasets(const QJsonArray& array) {
    QList<GraphInfo> datasets;
    datasets.reserve(array.size());

    // C++23 Ranges Transformation
    auto transform_range = array | std::views::transform([](const QJsonValue& val) {
        QJsonObject obj = val.toObject();
        return GraphInfo{
            .name = obj["name"].toString(),
            .description = obj["description"].toString(),
            .url = obj["url"].toString(),
            .filename = obj["filename"].toString(),
            .category = obj["category"].toString(),
            .numNodes = obj["nodes"].toVariant().toLongLong(),
            .numEdges = obj["edges"].toVariant().toLongLong(),
            .numTriangles = obj["triangles"].toVariant().toLongLong()
        };
    });

    std::ranges::copy(transform_range, std::back_inserter(datasets));
    return datasets;
}

QList<GraphInfo> SnapDatasetCache::loadBuiltInSnapshot() {
    return {
        { 
            .name = "Facebook Combined", 
            .description = "Social circles from Facebook",
            .url = "https://snap.stanford.edu/data/facebook_combined.txt.gz",
            .filename = "facebook_combined.txt",
            .category = "Social Networks",
            .numNodes = 4039, 
            .numEdges = 88234 
        },
        { 
            .name = "Enron Email Network", 
            .description = "Email communication network from Enron",
            .url = "https://snap.stanford.edu/data/email-Enron.txt.gz",
            .filename = "email-Enron.txt",
            .category = "Social Networks",
            .numNodes = 36692, 
            .numEdges = 183831 
        }
    };
}