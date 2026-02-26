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
    void graphDetailsReady(const QString& path, int nodes, int edges, double density);
    void error(const QString& message);
    void finished();

private:
    QString m_filePath;
};

#endif