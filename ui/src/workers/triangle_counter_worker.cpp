#include "triangle_counter_worker.h"
#include "TriangleCounting.h"
#include "density_algorithm.h"
#include <QDebug>
#include <QDateTime>
#include <QFileInfo>
#include <QElapsedTimer>
#include <string>

// התיקון הקריטי: הסרנו את ה-extern g_progressCallback שגרם לשגיאת הלינקר והטעינה

void TriangleCounterWorker::process() {
    try {
        QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
        emit progress(QString("[%1] Starting Triangle Counting...").arg(timestamp));

        // בדיקה שהקובץ קיים פיזית לפני הטעינה
        QFileInfo fileInfo(m_path);
        if (!fileInfo.exists()) {
            emit error(QString("File not found: %1").arg(m_path));
            return;
        }

        // יצירת פונקציית לוג מקומית שמעבירה הודעות ישירות ל-UI
        auto logger = [this](const std::string& msg) {
            emit progress(QString::fromStdString(msg));
        };

        QElapsedTimer timer;
        timer.start();

        std::string stdPath = m_path.toStdString();
        emit progress("Loading graph edges...");

        // שליחת ה-logger המקומי כפרמטר - זה מבטיח שהטעינה תצליח
        auto edges = load_edges(stdPath, logger);

        if (edges.empty()) {
            throw std::runtime_error("Graph is empty or format is invalid (no edges parsed).");
        }

        emit progress(QString("Loaded %1 edges in %2s")
            .arg(edges.size())
            .arg(timer.elapsed() / 1000.0));

        // הגדרת קונפיגורציה לאלגוריתם
        TriangleCountingConfig config;
        config.arboricity = m_arboricity;

        emit progress("Computing triangles (Importance Sampling)...");
        auto result = TriangleCounting::count(edges, config);

        // דיווח תוצאה סופית
        emit progress(QString("<b>Estimation: %1 triangles</b>").arg(static_cast<long long>(result.first)));

        emit finished(result.first);

    } catch (const std::exception& e) {
        emit error(QString("Error: %1").arg(e.what()));
    }
}