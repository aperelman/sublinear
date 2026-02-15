#ifndef GRAPH_INFO_H
#define GRAPH_INFO_H

#include <QString>
#include <cstdint>

struct GraphInfo {
    QString name;
    QString description;
    QString url;
    QString filename;
    QString category;
    QString localPath;
    QString fileSizeString;
    QString format;

    // Fixed-width 64-bit integers for cross-platform execution
    int64_t numNodes = 0;
    int64_t numEdges = 0;
    int64_t numTriangles = 0;

    double density = 0.0;
};

#endif // GRAPH_INFO_H