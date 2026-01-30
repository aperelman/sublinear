#pragma once

#include <QObject>
#include <QProcess>
#include <QString>

struct AlgorithmResult {
    bool success;
    double executionTime;
    QString output;
    QString errorMessage;
};

class AlgorithmRunner : public QObject {
    Q_OBJECT
    
public:
    explicit AlgorithmRunner(QObject* parent = nullptr);
    void runAlgorithm(const QString& algorithmType, 
                     const QString& graphFile,
                     int maxK);
    
signals:
    void finished(const AlgorithmResult& result);
    void progress(int current, int total);
    void error(const QString& errorMessage);
    
private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);
    void onReadyReadStandardOutput();
    
private:
    QProcess* process;
    QString currentAlgorithm;
    qint64 startTime;
    QString outputBuffer;
};
