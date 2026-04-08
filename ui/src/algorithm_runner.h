#ifndef ALGORITHM_RUNNER_H
#define ALGORITHM_RUNNER_H

#include <QObject>
#include <QString>
#include <memory>
#include <atomic>
#include <functional>

class GraphCache;

enum class ArboricityMethod {
    Exact,        // Dinic's max-flow (slow, precise)
    Approximate,  // Greedy peeling O(V+E) (fast, good approximation)
    Manual        // User-supplied value
};

class AlgorithmRunner : public QObject {
    Q_OBJECT
public:
    explicit AlgorithmRunner(QObject *parent = nullptr);
    ~AlgorithmRunner();

    void runTriangleCounting(const QString& filePath);
    void runArboricity(const QString& filePath, int degeneracy,
                       ArboricityMethod method = ArboricityMethod::Exact,
                       double manualValue = 0.0);
    void runImportanceSamplingEstimation(const QString& filePath, int64_t T_ref, double alpha_ref);
    void invalidateCache();
    void cancel();

    Q_SIGNALS:
    void logMessage(const QString &message);
    void finished(const QString &path, int64_t nodes, int64_t edges, int64_t triangles);
    void arboricityCalculated(double arboricity);
    void arboricityFailedZero();

private:
    QString normalizeGraphFile(const QString& filePath, std::function<void(const QString&)> log);

    std::shared_ptr<GraphCache> m_cache;
    std::shared_ptr<std::atomic<bool>> m_isDestroyed;
};

#endif
