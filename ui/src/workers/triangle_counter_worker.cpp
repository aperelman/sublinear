#include "triangle_counter_worker.h"
#include "TriangleCounting.h"
#include "density_algorithm.h"
#include "ExactArboricity.h"
#include <QDateTime>
#include <QFileInfo>
#include <QElapsedTimer>
#include <cmath>

void TriangleCounterWorker::process() {
    try {
        QString ts = QDateTime::currentDateTime().toString("hh:mm:ss");
        emit progress(QString("[%1] STARTING TRIANGLE COUNTING ANALYSIS").arg(ts));

        QFileInfo fi(m_path);
        if (!fi.exists()) {
            emit error(QString("File does not exist: %1").arg(m_path));
            return;
        }

        emit progress(QString("Target Graph: %1").arg(fi.fileName()));
        emit progress("─────────────────────────────────────");

        auto logFn = [this](const std::string& msg) {
            emit progress(QString::fromStdString(msg));
        };

        QElapsedTimer timer;
        timer.start();

        // Phase 1: Load edges once, reuse for both algorithms
        emit progress("<b>Phase 1: Loading Graph Data...</b>");
        auto edges = load_edges(m_path.toStdString(), logFn);

        if (edges.empty())
            throw std::runtime_error("No edges loaded. Check if the file is a valid edge list.");

        emit progress(QString("Successfully loaded %1 edges.").arg(edges.size()));

        // Phase 2: Compute exact arboricity first
        emit progress("─────────────────────────────────────");
        emit progress("<b>Phase 2: Computing Exact Arboricity...</b>");

        ArboricityOutput arboResult = ExactArboricity::compute(edges, logFn);
        double arboricity = arboResult.value;

        emit progress(QString("Arboricity (γ): %1").arg(arboricity));

        // Phase 3: Use arboricity for triangle counting
        emit progress("─────────────────────────────────────");
        emit progress("<b>Phase 3: Triangle Counting (Importance Sampling)...</b>");
        emit progress(QString("Using arboricity γ = %1 as sampling parameter").arg(arboricity));

        TriangleCountingConfig config;
        config.arboricity = arboricity;
        config.num_trials = 10000;
        config.seed       = 42;

        auto result_pair = TriangleCounting::count(edges, config);
        double estimated_triangles = result_pair.first;
        qint64 elapsedMs = timer.elapsed();

        ts = QDateTime::currentDateTime().toString("hh:mm:ss");
        emit progress("─────────────────────────────────────");
        emit progress(QString("[%1] ANALYSIS COMPLETE").arg(ts));
        emit progress(QString("Total Time: %1 seconds").arg(elapsedMs / 1000.0));
        emit progress(QString("Estimated Triangles: %1")
            .arg(static_cast<long long>(std::round(estimated_triangles))));
        emit progress("─────────────────────────────────────");

        emit finished(estimated_triangles);

    } catch (const std::exception& e) {
        emit error(QString("CRITICAL ERROR: %1").arg(e.what()));
    }
}
