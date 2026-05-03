#pragma once

#include <QObject>
#include <QDir>
#include <QJsonArray>
#include <QStringList>
#include <memory>

class AlgorithmRunner;

// Headless batch arboricity runner.
// Scans a directory for aga_L{layer}_H{head}.txt files,
// runs arboricity on each, writes results to a JSON file.
//
// Usage:
//   ./GraphAnalyzer --batch --input <dir> --output <json> [--method exact|approx]
//
class BatchRunner : public QObject {
    Q_OBJECT

public:
    explicit BatchRunner(const QString &inputDir,
                         const QString &outputPath,
                         const QString &method,   // "exact", "approx"
                         QObject *parent = nullptr);
    ~BatchRunner();

signals:
    void done(int exitCode);

public slots:
    void run();

private slots:
    void onArboricityCalculated(double arboricity);
    void onArboricityFailed();
    void onLogMessage(const QString &msg);

private:
    void processNext();
    void writeOutput();
    bool parseFilename(const QString &filename, int &layer, int &head) const;

    QString                         m_inputDir;
    QString                         m_outputPath;
    QString                         m_method;
    QStringList                     m_files;
    int                             m_currentIndex = 0;
    std::unique_ptr<AlgorithmRunner> m_runner;

    struct Result {
        int    layer;
        int    head;
        double arboricity;
        int    nodes;
        int    edges;
    };
    QVector<Result> m_results;
    int             m_total = 0;
};
