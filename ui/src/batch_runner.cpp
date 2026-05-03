#include "batch_runner.h"
#include "algorithm_runner.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QDebug>
#include <iostream>

BatchRunner::BatchRunner(const QString &inputDir,
                         const QString &outputPath,
                         const QString &method,
                         QObject *parent)
    : QObject(parent)
    , m_inputDir(inputDir)
    , m_outputPath(outputPath)
    , m_method(method)
    , m_runner(std::make_unique<AlgorithmRunner>())
{
    connect(m_runner.get(), &AlgorithmRunner::arboricityCalculated,
            this, &BatchRunner::onArboricityCalculated);
    connect(m_runner.get(), &AlgorithmRunner::arboricityFailedZero,
            this, &BatchRunner::onArboricityFailed);
    connect(m_runner.get(), &AlgorithmRunner::logMessage,
            this, &BatchRunner::onLogMessage);
}

BatchRunner::~BatchRunner() = default;

void BatchRunner::run() {
    QDir dir(m_inputDir);
    if (!dir.exists()) {
        std::cerr << "ERROR: Input directory does not exist: "
                  << m_inputDir.toStdString() << "\n";
        emit done(1);
        return;
    }

    // Collect all matching files: aga_L*_H*.txt
    QStringList allFiles = dir.entryList(QStringList() << "aga_L*_H*.txt",
                                          QDir::Files, QDir::Name);
    for (const QString &f : allFiles) {
        int layer, head;
        if (parseFilename(f, layer, head))
            m_files.append(dir.absoluteFilePath(f));
    }

    if (m_files.isEmpty()) {
        std::cerr << "ERROR: No aga_L*_H*.txt files found in "
                  << m_inputDir.toStdString() << "\n";
        emit done(1);
        return;
    }

    m_total = m_files.size();
    std::cout << "Found " << m_total << " graph files. Starting batch...\n";
    std::cout << "Method: " << m_method.toStdString() << "\n\n";

    m_currentIndex = 0;
    processNext();
}

void BatchRunner::processNext() {
    if (m_currentIndex >= m_files.size()) {
        writeOutput();
        emit done(0);
        return;
    }

    const QString &filePath = m_files[m_currentIndex];
    QString filename = QFileInfo(filePath).fileName();

    int layer, head;
    parseFilename(filename, layer, head);

    std::cout << QString("[%1/%2] L%3 H%4  %5\n")
                     .arg(m_currentIndex + 1).arg(m_total)
                     .arg(layer, 2, 10, QChar('0'))
                     .arg(head, 2, 10, QChar('0'))
                     .arg(filename)
                     .toStdString();

    m_runner->invalidateCache();

    ArboricityMethod method = ArboricityMethod::Exact;
    if (m_method == "approx")  method = ArboricityMethod::Approximate;

    m_runner->runArboricity(filePath, 0, method, 0.0);
}

void BatchRunner::onArboricityCalculated(double arboricity) {
    const QString &filePath = m_files[m_currentIndex];
    QString filename = QFileInfo(filePath).fileName();

    int layer, head;
    parseFilename(filename, layer, head);

    // We need node/edge counts — read from cache via a quick re-check.
    // AlgorithmRunner already loaded the graph; we emit finished(fp,n,e,0)
    // after arboricityCalculated. Store what we have, pick up n/e from
    // the finished signal if needed. For now store arboricity and move on.
    Result r;
    r.layer      = layer;
    r.head       = head;
    r.arboricity = arboricity;
    r.nodes      = 0;  // filled in onFinished if wired
    r.edges      = 0;
    m_results.append(r);

    std::cout << QString("  -> arboricity = %1\n").arg(arboricity, 0, 'f', 1).toStdString();

    ++m_currentIndex;
    processNext();
}

void BatchRunner::onArboricityFailed() {
    const QString &filePath = m_files[m_currentIndex];
    QString filename = QFileInfo(filePath).fileName();
    int layer, head;
    parseFilename(filename, layer, head);

    std::cerr << QString("  WARNING: arboricity=0 for %1, skipping\n")
                     .arg(filename).toStdString();

    Result r;
    r.layer      = layer;
    r.head       = head;
    r.arboricity = 0.0;
    r.nodes      = 0;
    r.edges      = 0;
    m_results.append(r);

    ++m_currentIndex;
    processNext();
}

void BatchRunner::onLogMessage(const QString &msg) {
    // Only print important lines to keep output clean
    if (msg.contains("arboricity", Qt::CaseInsensitive) ||
        msg.contains("ERROR", Qt::CaseInsensitive) ||
        msg.contains("bounds", Qt::CaseInsensitive)) {
        std::cout << "  " << msg.toStdString() << "\n";
    }
}

void BatchRunner::writeOutput() {
    // Sort results by layer then head
    std::sort(m_results.begin(), m_results.end(),
              [](const Result &a, const Result &b) {
                  return a.layer != b.layer ? a.layer < b.layer : a.head < b.head;
              });

    // Find grid dimensions
    int maxLayer = 0, maxHead = 0;
    for (const auto &r : m_results) {
        maxLayer = std::max(maxLayer, r.layer);
        maxHead  = std::max(maxHead,  r.head);
    }

    QJsonObject root;
    root["layers"]  = maxLayer + 1;
    root["heads"]   = maxHead  + 1;
    root["method"]  = m_method;
    root["total"]   = m_results.size();

    QJsonArray arr;
    for (const auto &r : m_results) {
        QJsonObject obj;
        obj["layer"]      = r.layer;
        obj["head"]       = r.head;
        obj["arboricity"] = r.arboricity;
        obj["nodes"]      = r.nodes;
        obj["edges"]      = r.edges;
        arr.append(obj);
    }
    root["arboricity"] = arr;

    // Build 2D grid for convenience
    // grid[layer][head] = arboricity value
    QJsonArray grid;
    for (int l = 0; l <= maxLayer; ++l) {
        QJsonArray row;
        for (int h = 0; h <= maxHead; ++h) {
            double val = 0.0;
            for (const auto &r : m_results)
                if (r.layer == l && r.head == h) { val = r.arboricity; break; }
            row.append(val);
        }
        grid.append(row);
    }
    root["grid"] = grid;

    QJsonDocument doc(root);
    QFile f(m_outputPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        std::cerr << "ERROR: Cannot write output: "
                  << m_outputPath.toStdString() << "\n";
        return;
    }
    f.write(doc.toJson(QJsonDocument::Indented));
    f.close();

    std::cout << "\nBatch complete. " << m_results.size()
              << " graphs processed.\n";
    std::cout << "Output written to: " << m_outputPath.toStdString() << "\n";
}

bool BatchRunner::parseFilename(const QString &filename, int &layer, int &head) const {
    // Matches: aga_L00_H00.txt  (zero-padded or not)
    static QRegularExpression re(R"(aga_L(\d+)_H(\d+)\.txt)");
    auto m = re.match(filename);
    if (!m.hasMatch()) return false;
    layer = m.captured(1).toInt();
    head  = m.captured(2).toInt();
    return true;
}
