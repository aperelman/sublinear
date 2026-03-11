#include "snap_dataset_cache.h"

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