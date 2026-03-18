#ifndef ALGORITHM_RUNNER_H
#define ALGORITHM_RUNNER_H

#include <QObject>
#include <QString>
#include <cstdint>
#include <memory>

// Do NOT include Snap.h here - AUTOMOC pulls headers into mocs_compilation.cpp
// which is compiled with C++23 and breaks on SNAP operator== ambiguities.
// GraphCache includes Snap.h only in its .cpp file.

// Forward declare GraphCache to avoid including Snap.h in this header
class GraphCache;

class AlgorithmRunner : public QObject {
    Q_OBJECT
public:
    explicit AlgorithmRunner(QObject *parent = nullptr);
    ~AlgorithmRunner();

    void runTriangleCounting(const QString& filePath);
    void runArboricity(const QString& filePath, int degeneracy);
    void runImportanceSamplingEstimation(const QString& filePath, int64_t T_ref, double alpha_ref);

    // Invalidate graph cache (call when user selects a new dataset)
    void invalidateCache();

Q_SIGNALS:
    void logRequest(const QString& message);
    void finished(const QString& name, int64_t nodes, int64_t edges, int64_t triangles);
    void arboricityFinished(double arboricity);
    void arboricityFailedZero();

private:
    std::unique_ptr<GraphCache> m_cache;
};

#endif
