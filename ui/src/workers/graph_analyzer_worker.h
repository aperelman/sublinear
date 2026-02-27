#ifndef GRAPH_ANALYZER_WORKER_H
#define GRAPH_ANALYZER_WORKER_H

#include <QObject>
#include <QString>

class GraphAnalyzerWorker : public QObject {
    Q_OBJECT
public:
    explicit GraphAnalyzerWorker(const QString& filePath, QObject* parent = nullptr);

public slots:
    void process();

    signals:
        void progress(const QString& message);
    void arboricityResult(double arboricity, int nodes, int edges);
    void error(const QString& message);
    void finished();

private:
    QString m_filePath;
};

#endif
