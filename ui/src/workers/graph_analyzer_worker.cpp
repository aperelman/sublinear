#include "workers/graph_analyzer_worker.h"
#include "ExactArboricity.h"
#include <QFileInfo>
#include <QFile>
#include <fstream>
#include <sstream>
#include <zlib.h>

GraphAnalyzerWorker::GraphAnalyzerWorker(const QString& filePath, QObject* parent)
    : QObject(parent), m_filePath(filePath) {}

static bool decompressGzip(const QString& gzPath, const QString& outPath) {
    gzFile gz = gzopen(gzPath.toLocal8Bit().constData(), "rb");
    if (!gz) return false;

    QFile outFile(outPath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        gzclose(gz);
        return false;
    }

    char buffer[8192];
    int bytesRead;
    while ((bytesRead = gzread(gz, buffer, sizeof(buffer))) > 0) {
        if (outFile.write(buffer, bytesRead) != bytesRead) {
            outFile.close();
            gzclose(gz);
            return false;
        }
    }
    outFile.close();
    gzclose(gz);
    return true;
}

static std::vector<std::pair<int, int>> loadEdges(const QString& filePath,
    std::function<void(const std::string&)> logger) {

    std::vector<std::pair<int, int>> edges;
    std::ifstream file(filePath.toStdString());

    if (!file.is_open()) {
        if (logger) logger("Failed to open file");
        return edges;
    }

    std::string line;
    int lineNum = 0;
    while (std::getline(file, line)) {
        lineNum++;
        if (line.empty() || line[0] == '#' || line[0] == '%') continue;

        std::istringstream iss(line);
        int u, v;
        if (iss >> u >> v) {
            if (u != v) {
                edges.emplace_back(u, v);
            }
        }
    }

    if (logger) logger("Loaded " + std::to_string(edges.size()) + " edges");
    return edges;
}

void GraphAnalyzerWorker::process() {
    try {
        emit progress("Starting Exact Arboricity Calculation...");

        if (!QFileInfo::exists(m_filePath)) {
            emit error("File does not exist: " + m_filePath);
            emit finished();
            return;
        }

        QString targetPath = m_filePath;

        // Handle .gz compression
        if (m_filePath.endsWith(".gz", Qt::CaseInsensitive)) {
            targetPath = m_filePath;
            targetPath.chop(3);

            if (!QFile::exists(targetPath)) {
                emit progress("Decompressing dataset...");
                if (!decompressGzip(m_filePath, targetPath)) {
                    emit error("Failed to decompress .gz file");
                    emit finished();
                    return;
                }
                emit progress("Decompression complete");
            }
        }

        // Load edges
        auto logger = [this](const std::string& msg) {
            emit progress(QString::fromStdString(msg));
        };

        emit progress("Loading graph edges...");
        auto edges = loadEdges(targetPath, logger);

        if (edges.empty()) {
            emit error("No edges loaded - file may be empty or invalid format");
            emit finished();
            return;
        }

        emit progress(QString("Loaded %1 edges").arg(edges.size()));

        // Calculate exact arboricity
        emit progress("Computing exact arboricity (Nash-Williams)...");
        auto result = ExactArboricity::compute(edges, logger);

        emit progress(QString("✓ Arboricity: %1").arg(result.arboricity));
        emit progress(QString("  Nodes: %1").arg(result.num_nodes));
        emit progress(QString("  Edges: %1").arg(edges.size()));

        emit arboricityResult(result.value, result.num_nodes, edges.size());
        emit finished();

    } catch (const std::exception& e) {
        emit error(QString("Error: %1").arg(e.what()));
        emit finished();
    }
}