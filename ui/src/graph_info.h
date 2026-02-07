#pragma once

#include <QString>
#include <QFileInfo>

struct GraphInfo {
    QString name;           // Display name
    QString filename;       // Full path
    QString format;         // "Edge List", "GraphML", etc.
    qint64 fileSize;        // Size in bytes
    QString description;    // Optional description
    
    // Cached statistics (loaded on demand)
    int vertices = -1;
    int edges = -1;
    bool statsLoaded = false;
    
    GraphInfo() = default;
    
    GraphInfo(const QString& filepath) {
        QFileInfo info(filepath);
        filename = filepath;
        name = info.fileName();
        fileSize = info.size();
        
        // Detect format from extension
        QString suffix = info.suffix().toLower();
        if (suffix == "txt" || suffix == "edges") {
            format = "Edge List";
        } else if (suffix == "graphml") {
            format = "GraphML";
        } else if (suffix == "gml") {
            format = "GML";
        } else {
            format = "Unknown";
        }
    }
    
    QString fileSizeString() const {
        if (fileSize < 1024) {
            return QString::number(fileSize) + " B";
        } else if (fileSize < 1024 * 1024) {
            return QString::number(fileSize / 1024.0, 'f', 1) + " KB";
        } else if (fileSize < 1024 * 1024 * 1024) {
            return QString::number(fileSize / (1024.0 * 1024.0), 'f', 1) + " MB";
        } else {
            return QString::number(fileSize / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB";
        }
    }
    
    QString statsString() const {
        if (statsLoaded) {
            return QString("V: %1, E: %2").arg(vertices).arg(edges);
        }
        return "Not loaded";
    }
};