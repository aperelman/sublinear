#include "graph_analyzer_worker.h"
#include <QDebug>
#include <QDateTime>
#include <QFileInfo>
#include <string>
#include <functional>
#include <cmath>

// Global callback for progress updates from C++ algorithm
std::function<void(const std::string&)> g_progressCallback;

// External function from density_algorithm.cpp
extern double calculateExactDensity(const std::string& filePath);

void GraphAnalyzerWorker::process() {
    try {
        QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
        
        emit progress(QString("[%1] Starting exact arboricity analysis...").arg(timestamp));
        emit progress(QString("File: %1").arg(m_path));
        
        // Verify file exists
        QFileInfo fileInfo(m_path);
        if (!fileInfo.exists()) {
            emit error(QString("File does not exist: %1").arg(m_path));
            return;
        }
        
        emit progress(QString("File size: %1 bytes").arg(fileInfo.size()));
        emit progress("─────────────────────────────────────");
        
        // Set up progress callback so C++ code can send updates
        g_progressCallback = [this](const std::string& msg) {
            emit progress(QString::fromStdString(msg));
        };
        
        // Run the algorithm (blocking call)
        emit progress("Running Goldberg's algorithm...");
        double result = calculateExactDensity(m_path.toStdString());
        
        // Clear callback
        g_progressCallback = nullptr;
        
        timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
        emit progress("─────────────────────────────────────");
        emit progress(QString("[%1] Calculation complete!").arg(timestamp));
        emit progress(QString("Maximum Average Degree: %1").arg(result, 0, 'f', 6));
        emit progress(QString("Arboricity (α₀): %1").arg(static_cast<int>(std::ceil(result))));
        emit progress("─────────────────────────────────────");
        
        // Return result
        emit finished(result);
        
    } catch (const std::exception& e) {
        g_progressCallback = nullptr;
        QString errorMsg = QString("ERROR: %1").arg(e.what());
        emit progress(errorMsg);
        emit error(errorMsg);
    } catch (...) {
        g_progressCallback = nullptr;
        QString errorMsg = "ERROR: Unknown exception occurred";
        emit progress(errorMsg);
        emit error(errorMsg);
    }
}
