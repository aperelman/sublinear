#include "graph_analyzer_worker.h"
#include <QDebug>
#include <string>

// External function from density_algorithm.cpp
extern double calculateExactDensity(const std::string& filePath);

void GraphAnalyzerWorker::process() {
    try {
        qDebug() << "Worker: Starting exact arboricity analysis for:" << m_path;
        
        // This is the blocking call that executes Goldberg's algorithm
        // It's called here so it doesn't freeze the main GUI thread.
        double result = calculateExactDensity(m_path.toStdString());
        
        qDebug() << "Worker: Calculation complete. Result:" << result;
        
        // Return the result to the SnapBrowserWidget via signal
        emit finished(result);
    } catch (const std::exception& e) {
        emit error(QString("Analysis failed: %1").arg(e.what()));
    } catch (...) {
        emit error("An unknown error occurred during exact calculation.");
    }
}