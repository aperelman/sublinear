#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSplitter>
#include <QTextEdit>
#include <QPushButton>
#include <QComboBox>
#include <QStatusBar>
#include <QMessageBox>
#include "graph_list_widget.h"
#include "graph_downloader.h"
#include "algorithm_runner.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onGraphDownloadRequested(const GraphInfo& graph);
    void onGraphAnalyzeRequested(const GraphInfo& graph);
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished(bool success, const QString& errorMessage);
    void onExtractionProgress(int percentage);
    void onAlgorithmFinished(bool success, const QString& result);

private:
    void setupUI();
    void updateGraphListAfterDownload();
    
    GraphListWidget* graphListWidget;
    QTextEdit* outputDisplay;
    QComboBox* algorithmSelector;
    QPushButton* runButton;
    
    GraphDownloader* downloader;
    AlgorithmRunner* algorithmRunner;
    
    QString currentDownloadingGraph;
};

#endif // MAINWINDOW_H
