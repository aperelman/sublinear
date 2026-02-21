#include "graph_analyzer_worker.h"
#include "density_algorithm.h"
#include <QDebug>
#include <QDateTime>
#include <QFileInfo>
#include <QElapsedTimer>
#include <string>
#include <functional>
#include <cmath>

// Global callback for progress updates from C++ algorithm
std::function<void(const std::string&)> g_progressCallback;

void GraphAnalyzerWorker::process() {
    try {
        QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");

        emit progress(QString("[%1] STARTING ANALYSIS").arg(timestamp));
        emit progress(QString("File: %1").arg(m_path));

        QFileInfo fileInfo(m_path);
        if (!fileInfo.exists()) {
            emit error(QString("File does not exist: %1").arg(m_path));
            return;
        }

        emit progress("Method: C++23 Nash-Williams Binary Search");
        emit progress("─────────────────────────────────────");

        g_progressCallback = [this](const std::string& msg) {
            emit progress(QString(">> %1").arg(QString::fromStdString(msg)));
        };

        QElapsedTimer timer;
        timer.start();

        // Calling the wrapper in density_algorithm.cpp
        double result = calculateExactDensity(m_path.toStdString());

        qint64 elapsedMs = timer.elapsed();
        g_progressCallback = nullptr;

        timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
        emit progress("─────────────────────────────────────");
        emit progress(QString("[%1] ANALYSIS COMPLETE").arg(timestamp));
        emit progress(QString("Total Time: %1 seconds").arg(elapsedMs / 1000.0));

        // Final results
        int gamma = static_cast<int>(result);
        emit progress(QString("Exact Arboricity (γ): %1").arg(gamma));
        emit progress(QString("Minimum Orientation Bound: %1").arg(gamma));
        emit progress(QString("Max Average Degree is in range: [%1, %2)").arg(gamma).arg(gamma * 2));
        emit progress("─────────────────────────────────────");

        emit finished(result);

    } catch (const std::exception& e) {
        g_progressCallback = nullptr;
        QString errorMsg = QString("CRITICAL ERROR: %1").arg(e.what());
        emit progress(errorMsg);
        emit error(errorMsg);
    }
}