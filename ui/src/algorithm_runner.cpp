#include "algorithm_runner.h"
#include "ArboricitySolver.h"
#include "ArboricityApprox.h"
#include "GraphCache.h"
#include "TriangleCounting.h"
#include <QTime>
#include <QFile>
#include <QtConcurrent/QtConcurrent>
#include <QMetaObject>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <algorithm>

QString AlgorithmRunner::normalizeGraphFile(const QString& filePath, std::function<void(const QString&)> log) {
    QFile file(filePath);
    if (!file.exists()) {
        log("Error: File not found: " + filePath);
        return QString();
    }
    return filePath;
}

AlgorithmRunner::AlgorithmRunner(QObject *parent)
    : QObject(parent)
    , m_cache(std::make_shared<GraphCache>())
    , m_isDestroyed(std::make_shared<std::atomic<bool>>(false))
{}

AlgorithmRunner::~AlgorithmRunner() {
    *m_isDestroyed = true;
}

void AlgorithmRunner::invalidateCache() {
    m_cache->invalidate();
}

void AlgorithmRunner::cancel() {
    *m_isDestroyed = true;
    // Re-create so future runs work
    m_isDestroyed = std::make_shared<std::atomic<bool>>(false);
}

void AlgorithmRunner::runTriangleCounting(const QString& filePath) {
    auto cache = m_cache;
    auto destroyed = m_isDestroyed;
    QtConcurrent::run([this, filePath, cache, destroyed]() {
        auto log = [&](const QString &m){
            if (*destroyed) return;
            QString msg = QTime::currentTime().toString("[HH:mm:ss] ") + m;
            QMetaObject::invokeMethod(this, [this, msg](){ Q_EMIT logMessage(msg); }, Qt::QueuedConnection);
        };
        QString path = normalizeGraphFile(filePath, log);
        if (path.isEmpty() || *destroyed) return;
        if (!cache->ensure(path.toStdString(), [&](const std::string& m){ log(QString::fromStdString(m)); })) return;
        Triangle::Result result = Triangle::countExact(cache->edges());
        log(QString("  Threads used: %1").arg(result.numThreads));
        if (!*destroyed) {
            QString fp = filePath;
            int64_t nodes = cache->nodeCount();
            int64_t edges = cache->edgeCount();
            int64_t triangles = result.numTriangles;
            QMetaObject::invokeMethod(this, [this, fp, nodes, edges, triangles](){
                Q_EMIT finished(fp, nodes, edges, triangles);
            }, Qt::QueuedConnection);
        }
    });
}

void AlgorithmRunner::runArboricity(const QString& filePath, int /*degeneracy*/,
                                    ArboricityMethod method, double manualValue) {
    auto cache = m_cache;
    auto destroyed = m_isDestroyed;
    QtConcurrent::run([this, filePath, cache, destroyed, method, manualValue]() {
        auto log = [&](const QString &m){
            if (*destroyed) return;
            QString msg = QTime::currentTime().toString("[HH:mm:ss] ") + m;
            QMetaObject::invokeMethod(this, [this, msg](){ Q_EMIT logMessage(msg); }, Qt::QueuedConnection);
        };
        QString path = normalizeGraphFile(filePath, log);
        if (path.isEmpty() || *destroyed) return;
        if (!cache->ensure(path.toStdString(), [&](const std::string& m){ log(QString::fromStdString(m)); })) return;

        double arboricity = 0.0;

        if (method == ArboricityMethod::Manual) {
            arboricity = manualValue;
            log(QString("Using manual arboricity value: %1").arg(arboricity));
        } else if (method == ArboricityMethod::Approximate) {
            log("Computing approximate arboricity (greedy peeling)...");
            const auto& edges = cache->edges();
            const int numNodes = static_cast<int>(cache->nodeCount());
            arboricity = static_cast<double>(ArboricityApprox::compute(
                edges, numNodes,
                [&](const std::string& m){ log(QString::fromStdString(m)); }
            ));
            log(QString("Approximate arboricity (greedy peeling) = %1").arg((int)arboricity));
        } else {
            const auto& edges = cache->edges();
            const int numNodes = static_cast<int>(cache->nodeCount());
            ArboricitySolver solver(numNodes);
            for (const auto& [u, v] : edges)
                solver.addEdge(u, v);
            int result = solver.computeExact(
                nullptr,
                [&](const std::string& m){ log(QString::fromStdString(m)); }
            );
            arboricity = static_cast<double>(result);
        }

        if (!*destroyed) {
            QString fp = filePath;
            int64_t nodes = cache->nodeCount();
            int64_t edgeCount = cache->edgeCount();
            double arb = arboricity;
            QMetaObject::invokeMethod(this, [this, fp, nodes, edgeCount, arb](){
                if (arb <= 0)
                    Q_EMIT arboricityFailedZero();
                else
                    Q_EMIT arboricityCalculated(arb);
                Q_EMIT finished(fp, nodes, edgeCount, 0);
            }, Qt::QueuedConnection);
        }
    });
}

void AlgorithmRunner::runImportanceSamplingEstimation(const QString& filePath, int64_t T_ref, double alpha_ref) {
    auto cache = m_cache;
    auto destroyed = m_isDestroyed;
    QtConcurrent::run([this, filePath, T_ref, alpha_ref, cache, destroyed]() {
        auto log = [&](const QString &m){
            if (*destroyed) return;
            QString msg = QTime::currentTime().toString("[HH:mm:ss] ") + m;
            QMetaObject::invokeMethod(this, [this, msg](){ Q_EMIT logMessage(msg); }, Qt::QueuedConnection);
        };

        QString path = normalizeGraphFile(filePath, log);
        if (path.isEmpty() || *destroyed) return;
        if (!cache->ensure(path.toStdString(), [&](const std::string& m){ log(QString::fromStdString(m)); })) return;

        const auto& edges = cache->edges();
        const int64_t m   = cache->edgeCount();
        const int64_t n   = cache->nodeCount();

        log("========================================");
        log("  IMPORTANCE SAMPLING — TRIANGLE ESTIMATION");
        log("  (Eden-Ron-Seshadhri two-phase algorithm)");
        log("========================================");
        log(QString("  Nodes: %1   Edges: %2").arg(n).arg(m));
        log(QString("  T_ref: %1   alpha: %2").arg(T_ref).arg(alpha_ref, 0, 'f', 3));

        if (m == 0) { log("ERROR: Empty graph."); return; }

        // ----------------------------------------------------------------
        // Phase 1: Uniform edge sparsification
        // Keep each edge with probability p = r/m.
        // C is adaptive: when T is small relative to alpha^3, variance is high
        // so we increase C to sample more edges and reduce error.
        // ----------------------------------------------------------------
        const double alpha3   = alpha_ref * alpha_ref * alpha_ref;
        const double ratio    = (double)std::max(T_ref, (int64_t)1) / std::max(alpha3, 1.0);
        const double C        = std::max(50.0, 500.0 / std::max(ratio, 0.001));
        const double t_cb     = std::pow((double)std::max(T_ref, (int64_t)1), 1.0 / 3.0);
        const double a_cb     = std::pow(std::max(alpha_ref, 1.0),            2.0 / 3.0);
        int64_t r_formula = (int64_t)std::ceil(C * (double)m / (t_cb * a_cb));
        int64_t r_min     = std::max((int64_t)1000, (int64_t)(0.20 * (double)m));
        int64_t r         = std::clamp(std::max(r_formula, r_min), (int64_t)1, m);
        double  p         = (double)r / (double)m;

        log("--- Phase 1: Edge Sparsification ---");
        if (C > 50.0)
            log(QString("  Adaptive C=%1 (T/α³=%2 is low — increasing sampling for accuracy)")
                    .arg(C, 0, 'f', 1).arg(ratio, 0, 'f', 4));
        else
            log(QString("  C=%1 (standard)").arg(C, 0, 'f', 1));
        log(QString("  Formula r=%1  floor r=%2  using r=%3  (p=%4)")
                .arg(r_formula).arg(r_min).arg(r).arg(p, 0, 'f', 4));

        std::mt19937_64 rng(std::random_device{}());
        std::uniform_real_distribution<double> uni(0.0, 1.0);

        std::vector<std::pair<int,int>> R;
        R.reserve(r);
        for (const auto& e : edges)
            if (uni(rng) < p)
                R.push_back(e);

        log(QString("  |R| = %1 edges sampled").arg((int64_t)R.size()));
        if (R.empty()) { log("WARNING: R is empty. Aborting."); return; }

        // ----------------------------------------------------------------
        // Build degree map and adjacency list on R
        // ----------------------------------------------------------------
        std::unordered_map<int,int> degR;
        degR.reserve(R.size() * 2);
        for (const auto& e : R) { degR[e.first]++; degR[e.second]++; }

        std::unordered_map<int, std::vector<int>> adjR;
        adjR.reserve(R.size() * 2);
        for (const auto& e : R) {
            adjR[e.first].push_back(e.second);
            adjR[e.second].push_back(e.first);
        }

        // Full-graph adjacency SET for O(1) closure checks
        // Store both directions so lookup works regardless of edge orientation.
        // The /2 factor in the estimator corrects for double-counting.
        std::unordered_map<int, std::unordered_set<int>> adjFull;
        adjFull.reserve(n);
        for (const auto& e : edges) {
            adjFull[e.first].insert(e.second);
            adjFull[e.second].insert(e.first);
        }

        // ----------------------------------------------------------------
        // Phase 2: Uniform wedge sampling from R, closure check in G
        //
        // w(e) = (deg_R(u)-1) + (deg_R(v)-1)  = wedge count through e in R
        // W(R) = sum of w(e)
        //
        // Sample a wedge uniformly from R:
        //   1. Pick edge e=(u,v) w.p. w(e)/W(R)
        //   2. Pick center c in {u,v} w.p. (deg_R(c)-1) / w(e)
        //   3. Pick tip t uniformly from N_R(center) \ {other}
        //   4. Check if (other--tip) is an edge in the FULL graph G
        //
        // Estimator:
        //   Wedge (center; other, tip) from R closes in G iff triangle exists.
        //   A triangle (a,b,c) contributes to closing when:
        //     - 2 of its edges are in R (the wedge), checked via W(R)
        //     - 3rd edge checked in G (always present if triangle exists)
        //   Each triangle has 3 such wedges, each surviving with prob p^2.
        //   E[closing/s] = 3 * T * p^2 / W(R)
        //   => T_hat = (closing/s) * W(R) / (3 * p^2)
        // ----------------------------------------------------------------
        log("--- Phase 2: Wedge Sampling ---");

        std::vector<double> weights;
        weights.reserve(R.size());
        double W_R = 0.0;
        for (const auto& e : R) {
            double w = (double)(std::max(degR[e.first],  1) - 1)
                     + (double)(std::max(degR[e.second], 1) - 1);
            weights.push_back(w);
            W_R += w;
        }

        log(QString("  W(R) = %1").arg(W_R, 0, 'f', 0));
        if (W_R == 0.0) { log("WARNING: No wedges in R. Aborting."); return; }

        const int64_t s = r * 5;
        int64_t closing = 0;

        std::discrete_distribution<int64_t> edgeDist(weights.begin(), weights.end());

        const int64_t progressInterval = s / 10;  // log every 10%
        for (int64_t i = 0; i < s; ++i) {
            if (progressInterval > 0 && i > 0 && i % progressInterval == 0)
                log(QString("  Wedge sampling progress: %1%").arg(i * 100 / s));
            int64_t idx = edgeDist(rng);
            int u = R[idx].first, v = R[idx].second;

            int wu = std::max(degR[u], 1) - 1;
            int wv = std::max(degR[v], 1) - 1;
            int total_uv = wu + wv;
            if (total_uv <= 0) continue;

            // Pick center proportional to (deg_R(c) - 1)
            int center, other;
            if (std::uniform_int_distribution<int>(0, total_uv - 1)(rng) < wu) {
                center = u; other = v;
            } else {
                center = v; other = u;
            }

            // Pick tip from N_R(center) \ {other} — single-pass, no bias
            const auto& nbrs = adjR[center];
            int validCount = (int)nbrs.size() - 1;
            if (validCount <= 0) continue;

            int tipIdx = std::uniform_int_distribution<int>(0, validCount - 1)(rng);
            int tip = -1, seen = 0;
            for (int nb : nbrs) {
                if (nb == other) continue;
                if (seen == tipIdx) { tip = nb; break; }
                ++seen;
            }
            if (tip == -1) continue;

            // Check closure in FULL graph G (not R)
            auto it = adjFull.find(other);
            if (it != adjFull.end() && it->second.count(tip))
                ++closing;
        }

        // Estimator derivation:
        // Each triangle {a,b,c} generates 6 ORDERED closing wedges in W(R):
        //   (b,a,c), (c,a,b), (a,b,c), (c,b,a), (a,c,b), (b,c,a)
        // Each survives into R with prob p^2 (both edges needed).
        // E[closing/s] = 6 * T * p^2 / W(R)
        // => T_hat = (closing/s) * W(R) / (6 * p^2)
        double T_hat = ((double)closing / (double)s)
                     * W_R
                     / (6.0 * p * p);

        log(QString("  Wedge samples s=%1  Closing=%2").arg(s).arg(closing));
        log(QString("  T_hat = %1").arg((int64_t)std::llround(T_hat)));
        log("========================================");

        if (!*destroyed) {
            QString fp  = filePath;
            int64_t est = (int64_t)std::llround(T_hat);
            QMetaObject::invokeMethod(this, [this, fp, n, m, est](){
                Q_EMIT finished(fp, n, m, est);
            }, Qt::QueuedConnection);
        }
    });
}