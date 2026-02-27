#ifndef TRIANGLE_COUNTER_WORKER_H
#define TRIANGLE_COUNTER_WORKER_H

#include <QObject>
#include <QString>
#include <cstdint>

class TriangleCounterWorker : public QObject {
    Q_OBJECT

public:
    // Declaration only - matches definition in .cpp
    explicit TriangleCounterWorker(const QString& filePath, QObject* parent = nullptr);

public slots:
    void process();

    signals:
        void progress(const QString& message);
    void graphDetailsReady(int64_t nodes, int64_t edges, double alpha);
    void finished(double estimate);
    void error(const QString& message);

private:
    QString m_filePath;
};

#endif