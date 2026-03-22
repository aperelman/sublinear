#ifndef SNAP_DATASET_CACHE_H
#define SNAP_DATASET_CACHE_H

#include <QObject>
#include <QMap>
#include <QString>
#include <cstdint>

struct DatasetStats {
    int64_t nodes          = 0;
    int64_t edges          = 0;
    int64_t triangles      = 0;   // SNAP reference (directed graph)
    int64_t exactTriangles = 0;   // computed exact count on undirected graph
    double  arboricity     = 0.0; // computed arboricity
    QString type;
    bool    isValid        = false;
};

class SnapDatasetCache : public QObject
{
    Q_OBJECT

public:
    explicit SnapDatasetCache(QObject *parent = nullptr);

    void addDataset(const QString &name, const DatasetStats &stats);
    DatasetStats getDataset(const QString &name) const;
    bool hasDataset(const QString &name) const;
    QStringList getAllDatasetNames() const;

    // Persist to / load from a JSON file keyed by file path
    bool save(const QString &jsonPath) const;
    bool load(const QString &jsonPath);

Q_SIGNALS:
    void datasetSelected(const QString &name, const DatasetStats &stats);
    void analysisRequested(const QString &name);

private:
    QMap<QString, DatasetStats> m_cache;
};

#endif // SNAP_DATASET_CACHE_H
