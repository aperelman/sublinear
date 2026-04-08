#include "snap_dataset_cache.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

SnapDatasetCache::SnapDatasetCache(QObject *parent)
    : QObject(parent)
{
}

void SnapDatasetCache::addDataset(const QString &name, const DatasetStats &stats)
{
    m_cache[name] = stats;
}

DatasetStats SnapDatasetCache::getDataset(const QString &name) const
{
    return m_cache.value(name);
}

bool SnapDatasetCache::hasDataset(const QString &name) const
{
    return m_cache.contains(name);
}

QStringList SnapDatasetCache::getAllDatasetNames() const
{
    return m_cache.keys();
}

bool SnapDatasetCache::save(const QString &jsonPath) const
{
    QJsonObject root;
    for (auto it = m_cache.constBegin(); it != m_cache.constEnd(); ++it) {
        const DatasetStats& s = it.value();
        QJsonObject obj;
        obj["nodes"]          = (qint64)s.nodes;
        obj["edges"]          = (qint64)s.edges;
        obj["triangles"]      = (qint64)s.triangles;
        obj["exactTriangles"] = (qint64)s.exactTriangles;
        obj["arboricity"]     = s.arboricity;
        obj["type"]           = s.type;
        obj["isValid"]        = s.isValid;
        root[it.key()] = obj;
    }
    QFile f(jsonPath);
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(QJsonDocument(root).toJson());
    return true;
}

bool SnapDatasetCache::load(const QString &jsonPath)
{
    QFile f(jsonPath);
    if (!f.open(QIODevice::ReadOnly)) return false;
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) return false;

    QJsonObject root = doc.object();
    for (auto it = root.constBegin(); it != root.constEnd(); ++it) {
        QJsonObject obj = it.value().toObject();
        DatasetStats s;
        s.nodes          = (int64_t)obj["nodes"].toDouble();
        s.edges          = (int64_t)obj["edges"].toDouble();
        s.triangles      = (int64_t)obj["triangles"].toDouble();
        s.exactTriangles = (int64_t)obj["exactTriangles"].toDouble();
        s.arboricity     = obj["arboricity"].toDouble();
        s.type           = obj["type"].toString();
        s.isValid        = obj["isValid"].toBool();
        m_cache[it.key()] = s;
    }
    return true;
}
