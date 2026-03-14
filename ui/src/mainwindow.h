#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QPlainTextEdit>
#include <QLabel>
#include <QUrl>
#include <memory>
#include "algorithm_runner.h"
#include "snap_browser_widget.h"
#include "download_manager.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private Q_SLOTS:
    void handleRunClicked();
    void updateProperties(const QString& name, int64_t nodes, int64_t edges, int64_t triangles);

private:
    void runAlgorithmOnFile(const QString &filePath);
    QString localPathForDataset(const QString &name) const;
    static bool decompressGz(const QString &gzPath, const QString &outPath);

    QLineEdit       *m_editFilePath;
    QComboBox       *m_algoSelection;
    QPushButton     *m_btnRun;
    QLabel          *m_labelNodes;
    QLabel          *m_labelEdges;
    QLabel          *m_labelTriangles;
    QPlainTextEdit  *m_textLog;

    // SNAP dataset selection state
    QString m_pendingSnapName;
    QUrl    m_pendingSnapUrl;

    // Custom Components
    SnapBrowserWidget                *m_snapBrowser;
    std::unique_ptr<DownloadManager>  m_downloadManager;
    std::unique_ptr<AlgorithmRunner>  m_runner;
};

#endif
