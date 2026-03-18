#include "algorithm_runner.h"
#include <QTime>
#include "ExactArboricity.h"
#include "GraphCache.h"
#include "TriangleCounting.h"
#include <QFile>
#include <QThreadPool>
#ifdef forever
#undef forever
#endif
#include <Snap.h>
#include <random>
#include <vector>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <algorithm>
#include <chrono>

AlgorithmRunner::AlgorithmRunner(QObject *parent)
    : QObject(parent)
    , m_cache(std::make_unique<GraphCache>())
{}

AlgorithmRunner::~AlgorithmRunner() = default;

void AlgorithmRunner::invalidateCache() {
    m_cache->invalidate();
}

// ---------------------------------------------------------------------------
// Timing helper
// ---------------------------------------------------------------------------
using Clock = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<Clock>;

static double elapsedMs(TimePoint start) {
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

// ---------------------------------------------------------------------------
// Generic graph file normalizer
// ---------------------------------------------------------------------------
static QString normalizeGraphFile(const QString &filePath,
                                  std::function<void(const QString&)> log)
{
    log("--- Phase: Format Detection ---");
    log("Inspecting file: " + filePath);

    std::ifstream in(filePath.toStdString());
    if (!in.is_open()) {
        log("ERROR: Cannot open file: " + filePath);
        return {};
    }

    std::string peekLine;
    int skipped = 0;
    while (std::getline(in, peekLine)) {
        if (peekLine.empty() || peekLine[0] == '#' || peekLine[0] == '%') { skipped++; continue; }
        break;
    }
    in.seekg(0);

    log(QString("Skipped %1 comment/empty lines. First data line: \"%2\"")
            .arg(skipped).arg(QString::fromStdString(peekLine).left(80)));

    bool isSrcTgt = (peekLine.rfind("SRC:", 0) == 0 ||
                     peekLine.rfind("TGT:", 0) == 0);

    bool isNamedEdgeList = false;
    if (!isSrcTgt) {
        std::istringstream ss(peekLine);
        std::string a, b;
        ss >> a >> b;
        if (!a.empty() && !b.empty()) {
            bool aInt = std::all_of(a.begin(), a.end(), [](char c){ return std::isdigit(c) || c == '-'; });
            bool bInt = std::all_of(b.begin(), b.end(), [](char c){ return std::isdigit(c) || c == '-'; });
            if (!aInt || !bInt) isNamedEdgeList = true;
        }
    }

    if (!isSrcTgt && !isNamedEdgeList) {
        log("Format: standard integer edge list — no normalization needed.");
        return filePath;
    }

    log(isSrcTgt ? "Format: SRC/TGT record style — normalizing to integer edge list..."
                 : "Format: named edge list — mapping names to integer IDs...");

    auto t0 = Clock::now();
    std::unordered_map<std::string, int> nameToId;
    std::vector<std::pair<int,int>> edges;
    int nextId = 0;

    auto getId = [&](const std::string &name) -> int {
        auto it = nameToId.find(name);
        if (it != nameToId.end()) return it->second;
        nameToId[name] = nextId;
        return nextId++;
    };

    std::string line;
    if (isSrcTgt) {
        std::string src, tgt;
        while (std::getline(in, line)) {
            if (line.rfind("SRC:", 0) == 0) src = line.substr(4);
            else if (line.rfind("TGT:", 0) == 0) {
                tgt = line.substr(4);
                if (!src.empty() && !tgt.empty())
                    edges.push_back({getId(src), getId(tgt)});
            }
        }
    } else {
        while (std::getline(in, line)) {
            if (line.empty() || line[0] == '#' || line[0] == '%') continue;
            std::istringstream ss(line);
            std::string a, b;
            if (ss >> a >> b)
                edges.push_back({getId(a), getId(b)});
        }
    }

    if (edges.empty()) {
        log("ERROR: No edges found after parsing — check file format.");
        return {};
    }

    // Deduplicate for undirected graph: normalise each edge so u < v,
    // then remove duplicates (handles both (u,v) and (v,u) in directed input,
    // and also removes self-loops where u == v).
    for (auto &[u, v] : edges)
        if (u > v) std::swap(u, v);

    std::sort(edges.begin(), edges.end());
    edges.erase(std::unique(edges.begin(), edges.end()), edges.end());
    // Remove self-loops
    edges.erase(std::remove_if(edges.begin(), edges.end(),
        [](const std::pair<int,int> &e){ return e.first == e.second; }),
        edges.end());

    log(QString("Normalization: %1 unique undirected edges, %2 unique nodes in %3 ms.")
            .arg(edges.size()).arg(nameToId.size()).arg(elapsedMs(t0), 0, 'f', 1));

    QString tmpPath = filePath + ".normalized.txt";
    std::ofstream out(tmpPath.toStdString());
    if (!out.is_open()) {
        log("ERROR: Cannot write temp file: " + tmpPath);
        return {};
    }
    out << "# Normalized edge list\n";
    for (auto &[u, v] : edges)
        out << u << " " << v << "\n";
    out.close();

    log("Normalization complete → " + tmpPath);
    return tmpPath;
}

// ---------------------------------------------------------------------------
// Exact Triangle Counting
// ---------------------------------------------------------------------------
void AlgorithmRunner::runTriangleCounting(const QString& filePath) {
    QThreadPool::globalInstance()->start([this, filePath]() {
        try {
            auto log = [this](const QString &m){
                QString ts = QTime::currentTime().toString("[HH:mm:ss] ");
                Q_EMIT logRequest(ts + m);
            };
            auto total = Clock::now();

            log("========================================");
            log("  EXACT TRIANGLE COUNTING");
            log("========================================");

            // Phase 1: format detection / normalization
            QString path = normalizeGraphFile(filePath, log);
            if (path.isEmpty()) return;

            // Phase 2: load graph (uses cache if already loaded)
            auto cacheLog = [&](const std::string& m){ log(QString::fromStdString(m)); };
            if (!m_cache->ensure(path.toStdString(), cacheLog)) {
                log("ERROR: Failed to load graph.");
                return;
            }
            int64_t nodes = m_cache->nodeCount();
            int64_t edges = m_cache->edgeCount();
            const auto& edgeList = m_cache->edges();

            if (nodes == 0 || edges == 0) {
                log("ERROR: Empty graph — file may be in an unsupported format.");
                return;
            }

            // Update properties panel immediately with real graph counts
            Q_EMIT finished(filePath, nodes, edges, 0);

            // Phase 4: Chiba-Nishizeki degeneracy-ordered triangle counting O(m*alpha)
            log("--- Phase: Triangle Counting (Chiba-Nishizeki, O(m·α)) ---");
            log("Degeneracy ordering + marking — much faster than O(m·√m)...");
            auto t = Clock::now();

            int progressInterval = std::max(1LL, nodes / 20);
            int64_t lastReported = 0;
            auto onProgress = [&](int64_t triSoFar, int64_t nodesDone, int64_t totalNodes) {
                if (nodesDone - lastReported >= progressInterval) {
                    lastReported = nodesDone;
                    int pct = static_cast<int>(100.0 * nodesDone / totalNodes);
                    log(QString("  %1% done (%2/%3 nodes) — triangles so far: %4")
                        .arg(pct).arg(nodesDone).arg(totalNodes).arg(triSoFar));
                }
            };

            Triangle::Result tcResult = Triangle::countExact(edgeList, onProgress);
            int64_t triangles = tcResult.numTriangles;
            double ms = elapsedMs(t);

            log(QString("Counting complete in %1 ms.").arg(ms, 0, 'f', 1));
            log("----------------------------------------");
            log(QString("RESULT  triangles : %1").arg(triangles));
            log(QString("        nodes     : %1").arg(nodes));
            log(QString("        edges     : %1").arg(edges));
            log(QString("Total elapsed     : %1 ms").arg(elapsedMs(total), 0, 'f', 1));
            log("========================================");

            // Update properties panel with final triangle count
            Q_EMIT finished(filePath, nodes, edges, triangles);

            if (path != filePath) QFile::remove(path);
        } catch (const std::exception& e) {
            Q_EMIT logRequest(QString("Error: %1").arg(e.what()));
        }
    });
}

// ---------------------------------------------------------------------------
// Exact Arboricity
// ---------------------------------------------------------------------------
void AlgorithmRunner::runArboricity(const QString& filePath, int degeneracy) {
    QThreadPool::globalInstance()->start([this, filePath, degeneracy]() {
        try {
            auto log = [this](const QString &m){
                QString ts = QTime::currentTime().toString("[HH:mm:ss] ");
                Q_EMIT logRequest(ts + m);
            };
            auto total = Clock::now();

            log("========================================");
            log("  EXACT ARBORICITY");
            log("========================================");

            // Phase 1: format detection / normalization
            QString path = normalizeGraphFile(filePath, log);
            if (path.isEmpty()) return;

            // Phase 2: load graph (uses cache if already loaded)
            auto cacheLog2 = [&](const std::string& m){ log(QString::fromStdString(m)); };
            if (!m_cache->ensure(path.toStdString(), cacheLog2)) {
                log("ERROR: Failed to load graph.");
                return;
            }
            int64_t nodes = m_cache->nodeCount();
            int64_t edges = m_cache->edgeCount();
            const auto& edgeList = m_cache->edges();

            if (nodes == 0 || edges == 0) {
                log("ERROR: Empty graph — file may be in an unsupported format.");
                return;
            }
            Q_EMIT finished(filePath, nodes, edges, 0);

            if (degeneracy > 0)
                log(QString("Degeneracy hint provided: %1").arg(degeneracy));
            else
                log("No degeneracy hint — algorithm will compute it automatically.");

            // Phase 4: exact arboricity
            log("--- Phase: Exact Arboricity (Nash-Williams / Goldberg push-relabel) ---");
            log("Running max-flow based forest decomposition...");
            auto t = Clock::now();

            auto logger = [this](const std::string& msg) {
                Q_EMIT logRequest(QString::fromStdString(msg));
            };
            ArboricityOutput out = ExactArboricity::compute(edgeList, degeneracy, logger);

            log(QString("Arboricity computation complete in %1 ms.").arg(elapsedMs(t), 0, 'f', 1));
            log("----------------------------------------");
            if (out.arboricity > 0) {
                log(QString("RESULT  arboricity : %1").arg(out.arboricity));
                log(QString("Total elapsed      : %1 ms").arg(elapsedMs(total), 0, 'f', 1));
            } else {
                log("WARNING: Arboricity computation returned 0.");
            }
            log("========================================");

            if (path != filePath) QFile::remove(path);

            if (out.arboricity <= 0) Q_EMIT arboricityFailedZero();
            else Q_EMIT arboricityFinished(out.arboricity);

        } catch (...) {
            Q_EMIT logRequest("Error processing arboricity.");
        }
    });
}

// ---------------------------------------------------------------------------
// Importance Sampling Triangle Estimation
// ---------------------------------------------------------------------------
void AlgorithmRunner::runImportanceSamplingEstimation(const QString& filePath,
                                                       int64_t T_ref,
                                                       double alpha_ref) {
    if (T_ref <= 0) {
        Q_EMIT logRequest("ABORT: Reference Triangle Count is 0. Run Exact Triangle Counting first.");
        return;
    }

    QThreadPool::globalInstance()->start([this, filePath, T_ref, alpha_ref]() {
        try {
            auto log = [this](const QString &m){
                QString ts = QTime::currentTime().toString("[HH:mm:ss] ");
                Q_EMIT logRequest(ts + m);
            };
            auto total = Clock::now();

            log("========================================");
            log("  IMPORTANCE SAMPLING — TRIANGLE ESTIMATION");
            log("========================================");
            log(QString("Input parameters:"));
            log(QString("  T_ref (reference triangle count) = %1").arg(T_ref));
            log(QString("  alpha (arboricity)               = %1").arg(alpha_ref));

            // Phase 1: format detection / normalization
            QString path = normalizeGraphFile(filePath, log);
            if (path.isEmpty()) return;

            // Phase 2: load graph (uses cache if already loaded)
            auto cacheLog3 = [&](const std::string& msg){ log(QString::fromStdString(msg)); };
            if (!m_cache->ensure(path.toStdString(), cacheLog3)) {
                log("ERROR: Failed to load graph.");
                return;
            }
            int64_t n = m_cache->nodeCount();
            int64_t m = m_cache->edgeCount();
            const auto& edgeList = m_cache->edges();

            if (n == 0 || m == 0) {
                log("ERROR: Empty graph — file may be in an unsupported format.");
                return;
            }
            Q_EMIT finished(filePath, n, m, 0);

            // Phase 3: compute sampling budget
            log("--- Phase: Sampling Budget Calculation ---");
            const double eps = 0.1;
            double t_prime = std::max(1.0, 0.1 * static_cast<double>(T_ref));
            int64_t r = static_cast<int64_t>((m * alpha_ref) / (eps * t_prime));
            if (r > m) r = m;
            if (r <= 0) r = 1;
            log(QString("  epsilon (eps)      = %1").arg(eps));
            log(QString("  t' = 0.1 * T_ref   = %1").arg(t_prime));
            log(QString("  r  = (m*alpha)/(eps*t') = (%1 * %2) / (%3 * %4) = %5")
                    .arg(m).arg(alpha_ref).arg(eps).arg(t_prime).arg(r));
            log(QString("  Sampling %1 edges out of %2 total (%3%)")
                    .arg(r).arg(m).arg(100.0 * r / m, 0, 'f', 1));

            // Phase 4: uniform edge sampling + common neighbour counting
            log("--- Phase: Uniform Edge Sampling ---");
            log(QString("Drawing %1 edges uniformly at random...").arg(r));
            log("For each sampled edge (u,v): counting common neighbours (triangles).");
            auto t = Clock::now();

            std::mt19937 gen(std::random_device{}());
            std::uniform_int_distribution<int64_t> dis(0, (int64_t)edgeList.size() - 1);

            int64_t triangleWeightSum = 0;
            int64_t lastBucket = -1;

            for (int64_t i = 0; i < r; ++i) {
                auto [u, v] = edgeList[dis(gen)];
                int common = 0;
                // Use cached adjacency list instead of SNAP graph
                for (int w : m_cache->neighbors(u)) {
                    if (m_cache->hasEdge(v, w)) common++;
                }
                triangleWeightSum += common;

                // Progress every 10%
                int64_t bucket = (i * 10) / r;
                if (bucket != lastBucket) {
                    lastBucket = bucket;
                    log(QString("  %1% done (%2/%3 samples) — cumulative weight = %4")
                            .arg(bucket * 10).arg(i).arg(r).arg(triangleWeightSum));
                }
            }

            log(QString("Sampling complete in %1 ms.").arg(elapsedMs(t), 0, 'f', 1));

            // Phase 6: final estimate
            log("--- Phase: Final Estimate ---");
            double estimate = static_cast<double>(triangleWeightSum) *
                              (static_cast<double>(m) / static_cast<double>(r)) / 3.0;
            log(QString("  Triangle weight sum        = %1").arg(triangleWeightSum));
            log(QString("  Scale factor m/r           = %1/%2 = %3")
                    .arg(m).arg(r).arg(static_cast<double>(m)/r, 0, 'f', 4));
            log("  Divide by 3 (each triangle is counted once per edge)");
            log("----------------------------------------");
            log(QString("RESULT  estimated triangles : %1").arg(static_cast<int64_t>(estimate)));
            log(QString("        reference T_ref     : %1").arg(T_ref));
            log(QString("        ratio est/ref       : %1")
                    .arg(static_cast<double>(estimate) / T_ref, 0, 'f', 3));
            log(QString("Total elapsed               : %1 ms").arg(elapsedMs(total), 0, 'f', 1));
            log("========================================");

            Q_EMIT finished(filePath, n, m, static_cast<int64_t>(estimate));

            if (path != filePath) QFile::remove(path);
        } catch (const std::exception& e) {
            Q_EMIT logRequest(QString("Error: %1").arg(e.what()));
        }
    });
}
