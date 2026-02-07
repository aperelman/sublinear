#include "algorithm_runner.h"
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCoreApplication>

AlgorithmRunner::AlgorithmRunner(QObject* parent)
    : QObject(parent)
    , process(new QProcess(this))
{
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &AlgorithmRunner::onProcessFinished);
    connect(process, &QProcess::errorOccurred,
            this, &AlgorithmRunner::onProcessError);
    connect(process, &QProcess::readyReadStandardOutput,
            this, &AlgorithmRunner::onReadyReadStandardOutput);
}

void AlgorithmRunner::runAlgorithm(const QString& algorithmType,
                                  const QString& graphFile,
                                  int maxK) {
    currentAlgorithm = algorithmType;
    startTime = QDateTime::currentMSecsSinceEpoch();
    outputBuffer.clear();
    
    QString program = "python3";
    QStringList arguments;
    arguments << "../../algorithms/python/arboricity/arboricity.py"
             << graphFile;
    
    process->start(program, arguments);
}

void AlgorithmRunner::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    qint64 endTime = QDateTime::currentMSecsSinceEpoch();
    double elapsed = (endTime - startTime) / 1000.0;
    
    AlgorithmResult result;
    result.executionTime = elapsed;
    result.success = (exitCode == 0 && exitStatus == QProcess::NormalExit);
    result.output = outputBuffer;
    
    if (!result.success) {
        result.errorMessage = process->readAllStandardError();
    }
    
    emit finished(result);
}

void AlgorithmRunner::onProcessError(QProcess::ProcessError error) {
    QString errorMsg = "Failed to run algorithm";
    emit this->error(errorMsg);
}

void AlgorithmRunner::onReadyReadStandardOutput() {
    QByteArray data = process->readAllStandardOutput();
    QString text = QString::fromUtf8(data);
    outputBuffer += text;
}
