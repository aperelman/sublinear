#include "graph_analyzer_worker.h"
#include "density_algorithm.h"
#include <QDateTime>
#include <QFileInfo>
#include <cmath>

void GraphAnalyzerWorker::process() {
    try {
        QString ts = QDateTime::currentDateTime().toString("hh:mm:ss");
        emit progress(QString("[%1] Starting exact arboricity analysis...").arg(ts));
        emit progress(QString("File: %1").arg(m_path));

        QFileInfo fi(m_path);
        if (!fi.exists()) {
            emit error(QString("File does not exist: %1").arg(m_path));
            return;
        }

        emit progress(QString("File size: %1 bytes").arg(fi.size()));
        emit progress("─────────────────────────────────────");

        auto logFn = [this](const std::string& msg) {
            emit progress(QString::fromStdString(msg));
        };

        double result = calculateExactDensity(m_path.toStdString(), logFn);

        ts = QDateTime::currentDateTime().toString("hh:mm:ss");
        emit progress("─────────────────────────────────────");
        emit progress(QString("[%1] Calculation complete!").arg(ts));
        emit progress(QString("Arboricity value: %1").arg(result, 0, 'f', 6));
        emit progress(QString("Arboricity (α₀): %1").arg(static_cast<int>(std::ceil(result))));
        emit progress("─────────────────────────────────────");

        emit finished(result);

    } catch (const std::exception& e) {
        emit error(QString("ERROR: %1").arg(e.what()));
    } catch (...) {
        emit error("ERROR: Unknown exception");
    }
}
