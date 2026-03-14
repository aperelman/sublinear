#include "algorithm_runner.h"
#include "ExactArboricity.h"
#include <QtConcurrent>
#include <Snap.h>
#include <random>
#include <vector>
#include <fstream>
#include <sstream>
#include <unordered_map>

AlgorithmRunner::AlgorithmRunner(QObject *parent) : QObject(parent) {}

// ---------------------------------------------------------------------------
// Generic graph file normalizer
//
// Detects format by peeking at the first non-comment line:
//   - "SRC:" / "TGT:" record style  → extract SRC/TGT pairs, map names to ints
//   - Two tokens, both integers      → standard edge list, pass through
//   - Two tokens, non-integer        → named edge list, map names to ints
//
// Returns path to a normalized edge list temp file (caller must delete it),
// or empty string on failure.
// ---------------------------------------------------------------------------
static QString normalizeGraphFile(const QString &filePath,
                                  std::function<void(const QString&)> log)
{
    std::ifstream in(filePath.toStdString());
    if (!in.is_open()) {
        log("ERROR: Cannot open file: " + filePath);
        return {};
    }

    // Peek at first non-comment, non-empty line to detect format
    std::string peekLine;
    while (std::getline(in, peekLine)) {
        if (peekLine.empty() || peekLine[0] == '#' || peekLine[0] == '%') continue;
        break;
    }
    in.seekg(0);

    // Detect format
    bool isSrcTgt = (peekLine.rfind("SRC:", 0) == 0 ||
                     peekLine.rfind("TGT:", 0) == 0);

    bool isNamedEdgeList = false;
    if (!isSrcTgt) {
        std::istringstream ss(peekLine);
        std::string a, b;
        ss >> a >> b;
        if (!a.empty() && !b.empty()) {
            // Check if both tokens are integers
            bool aInt = !a.empty() && std::all_of(a.begin(), a.end(),
                            [](char c){ return std::isdigit(c) || c == '-'; });
            bool bInt = !b.empty() && std::all_of(b.begin(), b.end(),
                            [](char c){ return std::isdigit(c) || c == '-'; });
            if (!aInt || !bInt) isNamedEdgeList = true;
        }
    }

    if (!isSrcTgt && !isNamedEdgeList) {
        log("Format detected: standard integer edge list — using directly.");
        return filePath; // No normalization needed
    }

    log(isSrcTgt ? "Format detected: SRC/TGT record style — normalizing..."
                 : "Format detected: named edge list — normalizing...");

    // Build normalized edge list
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
        // Named edge list
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

    log(QString("Parsed %1 edges, %2 nodes.")
            .arg(edges.size()).arg(nameToId.size()));

    // Write normalized temp file
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

void AlgorithmRunner::runTriangleCounting(const QString& filePath) {
    QtConcurrent::run([this, filePath]() {
        try {
            Q_EMIT logRequest("Detecting graph file format...");
            auto log = [this](const QString &m){ Q_EMIT logRequest(m); };
            QString path = normalizeGraphFile(filePath, log);
            if (path.isEmpty()) return;

            Q_EMIT logRequest("Loading graph for exact triangle counting...");
            PUNGraph Graph = TSnap::LoadEdgeList<PUNGraph>(path.toStdString().c_str(), 0, 1);
            int64_t nodes = Graph->GetNodes();
            int64_t edges = Graph->GetEdges();

            if (nodes == 0 || edges == 0) {
                Q_EMIT logRequest(QString("ERROR: Graph loaded with %1 nodes and %2 edges. "
                    "File may be empty or in an unsupported format.").arg(nodes).arg(edges));
                return;
            }

            Q_EMIT logRequest(QString("Graph loaded: %1 nodes, %2 edges. Computing triangles via TSnap::GetTriads...").arg(nodes).arg(edges));
            int64_t triangles = static_cast<int64_t>(TSnap::GetTriads(Graph));
            Q_EMIT logRequest(QString("TSnap::GetTriads result: %1 triangles.").arg(triangles));
            Q_EMIT finished(filePath, nodes, edges, triangles);

            if (path != filePath) QFile::remove(path); // clean up temp
        } catch (const std::exception& e) {
            Q_EMIT logRequest(QString("Error: %1").arg(e.what()));
        }
    });
}

void AlgorithmRunner::runArboricity(const QString& filePath, int degeneracy) {
    QtConcurrent::run([this, filePath, degeneracy]() {
        try {
            Q_EMIT logRequest("Detecting graph file format...");
            auto log = [this](const QString &m){ Q_EMIT logRequest(m); };
            QString path = normalizeGraphFile(filePath, log);
            if (path.isEmpty()) return;

            PUNGraph Graph = TSnap::LoadEdgeList<PUNGraph>(path.toStdString().c_str(), 0, 1);
            int64_t nodes = Graph->GetNodes();
            int64_t edges = Graph->GetEdges();

            if (nodes == 0 || edges == 0) {
                Q_EMIT logRequest(QString("ERROR: Graph loaded with %1 nodes and %2 edges. "
                    "File may be empty or in an unsupported format.").arg(nodes).arg(edges));
                return;
            }

            Q_EMIT logRequest(QString("Graph loaded: %1 nodes, %2 edges. Computing arboricity...").arg(nodes).arg(edges));

            std::vector<std::pair<int, int>> edgeList;
            edgeList.reserve(edges);
            for (TUNGraph::TEdgeI EI = Graph->BegEI(); EI < Graph->EndEI(); EI++)
                edgeList.push_back({EI.GetSrcNId(), EI.GetDstNId()});

            auto logger = [this](const std::string& msg) {
                Q_EMIT logRequest(QString::fromStdString(msg));
            };
            ArboricityOutput out = ExactArboricity::compute(edgeList, degeneracy, logger);

            if (path != filePath) QFile::remove(path);

            if (out.arboricity <= 0) Q_EMIT arboricityFailedZero();
            else Q_EMIT arboricityFinished(out.arboricity);

        } catch (...) {
            Q_EMIT logRequest("Error processing arboricity.");
        }
    });
}

void AlgorithmRunner::runImportanceSamplingEstimation(const QString& filePath, int64_t T_ref, double alpha_ref) {
    if (T_ref <= 0) {
        Q_EMIT logRequest("ABORT: Reference Triangle Count is 0. Estimation requires a baseline.");
        return;
    }

    QtConcurrent::run([this, filePath, T_ref, alpha_ref]() {
        try {
            Q_EMIT logRequest("Detecting graph file format...");
            auto log = [this](const QString &m){ Q_EMIT logRequest(m); };
            QString path = normalizeGraphFile(filePath, log);
            if (path.isEmpty()) return;

            Q_EMIT logRequest("Loading SNAP Graph for sampling...");
            PUNGraph Graph = TSnap::LoadEdgeList<PUNGraph>(path.toStdString().c_str(), 0, 1);

            int64_t m = Graph->GetEdges();
            int64_t nodes = Graph->GetNodes();

            if (nodes == 0 || m == 0) {
                Q_EMIT logRequest(QString("ERROR: Graph loaded with %1 nodes and %2 edges. "
                    "File may be empty or in an unsupported format.").arg(nodes).arg(m));
                return;
            }

            Q_EMIT logRequest(QString("Graph loaded: %1 nodes, %2 edges.").arg(nodes).arg(m));

            const double eps = 0.1;
            double t_prime = std::max(1.0, 0.1 * static_cast<double>(T_ref));
            int64_t r = static_cast<int64_t>((m * alpha_ref) / (eps * t_prime));
            if (r > m) r = m;
            if (r <= 0) r = 1;

            Q_EMIT logRequest(QString("Sampling Budget: %1 edges.").arg(r));

            std::vector<std::pair<int,int>> edgeList;
            edgeList.reserve(m);
            for (auto EI = Graph->BegEI(); EI < Graph->EndEI(); EI++)
                edgeList.push_back({EI.GetSrcNId(), EI.GetDstNId()});

            std::mt19937 gen(std::random_device{}());
            std::uniform_int_distribution<int64_t> dis(0, m - 1);

            int64_t triangleWeightSum = 0;
            for (int64_t i = 0; i < r; ++i) {
                auto [u, v] = edgeList[dis(gen)];
                int common = 0;
                auto NIu = Graph->GetNI(u);
                for (int j = 0; j < NIu.GetDeg(); ++j) {
                    int w = NIu.GetNbrNId(j);
                    if (Graph->IsEdge(v, w)) common++;
                }
                triangleWeightSum += common;
                if (i % 50000 == 0 && i > 0)
                    Q_EMIT logRequest(QString("Progress: %1%").arg((i * 100) / r));
            }

            double estimate = static_cast<double>(triangleWeightSum) *
                              (static_cast<double>(m) / static_cast<double>(r)) / 3.0;
            Q_EMIT logRequest(QString("Estimation complete. Estimated triangles: %1").arg(static_cast<int64_t>(estimate)));
            Q_EMIT finished(filePath, nodes, m, static_cast<int64_t>(estimate));

            if (path != filePath) QFile::remove(path);
        } catch (const std::exception& e) {
            Q_EMIT logRequest(QString("Error: %1").arg(e.what()));
        }
    });
}