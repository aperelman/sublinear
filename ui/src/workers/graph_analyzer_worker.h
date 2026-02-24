#pragma once

#include <QObject>
#include <QString>

class GraphAnalyzerWorker : public QObject {
    Q_OBJECT

public:
    explicit GraphAnalyzerWorker(const QString& path, QObject* parent = nullptr)
        : QObject(parent), m_path(path) {}

public slots:
    void process();

signals:
    void finished(double result);
    void progress(const QString& message);
    void error(const QString& message);

private:
    QString m_path;
};
