#ifndef TRIANGLE_COUNTER_WORKER_H
#define TRIANGLE_COUNTER_WORKER_H

#include <QObject>
#include <QString>

class TriangleCounterWorker : public QObject {
    Q_OBJECT
public:
    explicit TriangleCounterWorker(const QString& filePath, QObject* parent = nullptr);

public slots:
    void process();

    signals:
        void progress(const QString& message);
    void graphDetailsReady(const QString& path, int nodes, int edges, int triangles);
    void error(const QString& message);
    void finished();

private:
    QString m_filePath;
};

#endif