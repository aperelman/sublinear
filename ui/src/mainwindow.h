#pragma once

#include <QMainWindow>
#include <QTabWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QComboBox>
#include <QSpinBox>
#include "graph_list_widget.h"
#include "snap_browser_widget.h"
#include "snap_dataset_cache.h"
#include "algorithm_runner.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();
    
private slots:
    void onGraphSelected(const GraphInfo& graph);
    void onGraphDoubleClicked(const GraphInfo& graph);
    void onDatasetReady(const QString& filePath);
    void onRunAlgorithmClicked();
    void onAlgorithmFinished(const AlgorithmResult& result);
    void onAlgorithmProgress(int current, int total);
    void onAlgorithmError(const QString& error);
    
private:
    void setupUI();
    void setupMenuBar();
    void updateStatusBar(const QString& message);
    void loadSnapDatasets();
    
    GraphListWidget* graphListWidget;
    SnapBrowserWidget* snapBrowserWidget;
    QTabWidget* leftTabWidget;
    QTabWidget* tabWidget;
    QComboBox* algorithmCombo;
    QSpinBox* maxKSpinBox;
    QPushButton* runButton;
    QProgressBar* progressBar;
    QTextEdit* resultsText;
    AlgorithmRunner* algorithmRunner;
    GraphInfo currentGraphInfo;
};
