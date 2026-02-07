#pragma once

#include <QString>
#include <QVector>
#include <QJsonDocument>
#include "snap_catalog.h"

class SnapDatasetCache {
public:
    // Save datasets to cache file
    static bool saveToCache(const QVector<SnapDataset>& datasets);
    
    // Load datasets from cache file
    static QVector<SnapDataset> loadFromCache();
    
    // Check if cache exists
    static bool cacheExists();
    
    // Get cache file path
    static QString getCachePath();
    
    // Get cache timestamp
    static QDateTime getCacheTimestamp();
    
    // Load built-in snapshot (fallback)
    static QVector<SnapDataset> loadBuiltInSnapshot();
    
private:
    static QJsonDocument datasetsToJson(const QVector<SnapDataset>& datasets);
    static QVector<SnapDataset> jsonToDatasets(const QJsonDocument& doc);
};
