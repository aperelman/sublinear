#pragma once

#include <QString>
#include <QVector>

struct SnapDataset {
    QString name;
    QString description;
    QString url;
    QString filename;
    int nodes;
    int edges;
    QString category;
    
    QString displayName() const {
        return QString("%1 (V: %2, E: %3)")
            .arg(name)
            .arg(nodes)
            .arg(edges);
    }
};
