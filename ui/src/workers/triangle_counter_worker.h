#pragma once
#include <QObject>
#include <QString>

class TriangleCounterWorker : public QObject {
    Q_OBJECT
public:
    explicit TriangleCounterWorker(const QString& path, QObject* parent = nullptr)
        : QObject(parent), m_path(path) {}

public slots:
    void process();

signals:
    void progress(const QString& message);
    void finished(double result);
    void error(const QString& message);

private:
    QString m_path;
};
