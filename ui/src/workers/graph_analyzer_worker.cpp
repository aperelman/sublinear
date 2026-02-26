#include "workers/graph_analyzer_worker.h"
#include "density_algorithm.h"
#include <QFileInfo>

GraphAnalyzerWorker::GraphAnalyzerWorker(const QString& filePath, QObject* parent)
    : QObject(parent), m_filePath(filePath) {}

void GraphAnalyzerWorker::process() {
    emit progress("Analyzing graph density...");

    if (!QFileInfo::exists(m_filePath)) {
        emit error("File does not exist: " + m_filePath);
        emit finished();
        return;
    }

    std::string stdPath = m_filePath.toStdString();

    // Using the namespace defined in density_algorithm.h
    auto adj = Arboricity::loadGraph(stdPath);

    if (adj.empty()) {
        emit error("Failed to load graph or graph is empty.");
        emit finished();
        return;
    }

    Arboricity::GraphAnalysisResult result = Arboricity::analyzeAndCount(adj, 0.0);

    emit graphDetailsReady(m_filePath, (int)result.numNodes, (int)result.numEdges, result.density);
    emit finished();
}