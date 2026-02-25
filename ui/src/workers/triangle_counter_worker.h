#pragma once
#include <QObject>
#include <QString>

class TriangleCounterWorker : public QObject {
    Q_OBJECT
public:
    explicit TriangleCounterWorker(const QString& path,
                                   long long T_snap = 0,
                                   QObject* parent = nullptr)
        : QObject(parent), m_path(path), m_T_snap(T_snap), m_arboricity(0.0) {}

    // פונקציה להעברת ערך האבוריסיטי מה-UI ל-Worker
    void setArboricity(double arb) { m_arboricity = arb; }

public slots:
    void process();

    signals:
        void progress(const QString& message);
    void finished(double result);
    void error(const QString& message);

private:
    QString   m_path;
    long long m_T_snap;
    double    m_arboricity; // המשתנה עבור שגיאה C2065
};