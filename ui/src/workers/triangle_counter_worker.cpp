#include "workers/triangle_counter_worker.h"
#include "TriangleCounting.h"
#include <QFileInfo>

TriangleCounterWorker::TriangleCounterWorker(const QString& filePath, QObject* parent)
    : QObject(parent), m_filePath(filePath) {}

void TriangleCounterWorker::process() {
    emit progress("Counting triangles...");

    if (!QFileInfo::exists(m_filePath)) {
        emit error("File does not exist: " + m_filePath);
        emit finished();
        return;
    }

    auto adj = Triangle::loadGraph(m_filePath.toStdString());

    if (adj.empty()) {
        emit error("Failed to load graph.");
        emit finished();
        return;
    }

    auto result = Triangle::analyzeAndCount(adj, 0.0);

    emit graphDetailsReady(
        m_filePath,
        static_cast<int>(result.numNodes),
        static_cast<int>(result.numEdges),
        static_cast<int>(result.numTriangles)
    );

    emit finished();
}