#ifndef ALGORITHM_RUNNER_H
#define ALGORITHM_RUNNER_H

#include <QObject>
#include <QProcess>
#include <QString>
#include "graph_info.h"

class AlgorithmRunner : public QObject {
    Q_OBJECT

public:
    explicit AlgorithmRunner(QObject *parent = nullptr);
    ~AlgorithmRunner();

    void runAlgorithm(const QString& algorithmName, const GraphInfo& graph);
    bool isRunning() const { return process != nullptr && process->state() == QProcess::Running; }

signals:
    void finished(bool success, const QString& result);
    void outputReceived(const QString& output);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();

private:
    QProcess* process;
    QString currentAlgorithm;
};

#endif // ALGORITHM_RUNNER_H
