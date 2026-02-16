#ifndef SNAP_DATASET_CACHE_H
#define SNAP_DATASET_CACHE_H

#include <QList>
#include <QString>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>

struct GraphInfo;
class SnapDatasetCache {
public:
    [[nodiscard]] static bool saveToCache(const QList<GraphInfo>& datasets);
    static QList<GraphInfo> loadFromCache();
    static bool cacheExists();
    static QDateTime getCacheTimestamp();

private:
    // Helper for saving: Renamed to avoid return-type conflict with a 'toJson' style name
    static QJsonDocument datasetsToDocument(const QList<GraphInfo>& datasets);
    static QJsonArray datasetsToSnapshot(const QList<GraphInfo>& datasets);
    
    // Overloaded Helpers for loading: Allows passing either Document or Array
    static QList<GraphInfo> jsonToDatasets(const QJsonDocument& doc);
    static QList<GraphInfo> jsonToDatasets(const QJsonArray& array);

    static QList<GraphInfo> loadBuiltInSnapshot();
    static QString getCachePath();
};

#endif // SNAP_DATASET_CACHE_H