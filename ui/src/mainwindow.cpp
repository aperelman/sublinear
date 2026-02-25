#include "mainwindow.h"
#include "graph_list_widget.h"
#include "snap_browser_widget.h"
#include "workers/triangle_counter_worker.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QStatusBar>
#include <QComboBox>
#include <QPushButton>
#include <QTextEdit>
#include <QTabWidget>
#include <QFont>
#include <QTextCursor>
#include <QThread>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupUI();
    setupMenuBar();
    networkManager = new QNetworkAccessManager(this);
    connect(networkManager, &QNetworkAccessManager::finished,
            this, &MainWindow::handleNetworkReply);
    loadSnapDatasets();
}

void MainWindow::setupUI() {
    setWindowTitle("GraphAnalyzer");
    resize(1200, 800);

    auto* centralWidget = new QWidget(this);
    auto* mainLayout = new QHBoxLayout(centralWidget);

    leftTabWidget     = new QTabWidget();
    graphListWidget   = new GraphListWidget();
    snapBrowserWidget = new SnapBrowserWidget();

    leftTabWidget->addTab(graphListWidget,   "Local Files");
    leftTabWidget->addTab(snapBrowserWidget, "SNAP Datasets");
    mainLayout->addWidget(leftTabWidget);

    auto* rightPanel  = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightPanel);

    auto* algoGroup  = new QGroupBox("Algorithm Selection");
    auto* algoLayout = new QVBoxLayout(algoGroup);

    algorithmCombo = new QComboBox();
    algorithmCombo->addItem("-- Select Algorithm --", "");
    algorithmCombo->addItem("Exact Arboricity Calculate", "exact_arboricity");
    algorithmCombo->addItem("Triangle Counting",          "triangle_counting");

    algoLayout->addWidget(new QLabel("Algorithm:"));
    algoLayout->addWidget(algorithmCombo);

    runButton = new QPushButton("Run");
    runButton->setEnabled(false);
    algoLayout->addWidget(runButton);

    rightLayout->addWidget(algoGroup);

    resultsText = new QTextEdit();
    resultsText->setReadOnly(true);
    resultsText->setFont(QFont("Monospace", 9));
    resultsText->setStyleSheet("QTextEdit { background-color: #1e1e1e; color: #d4d4d4; }");
    rightLayout->addWidget(resultsText);

    mainLayout->addWidget(rightPanel, 1);
    setCentralWidget(centralWidget);

    connect(algorithmCombo,   &QComboBox::currentIndexChanged,
            this,             &MainWindow::checkRunRequirements);
    connect(snapBrowserWidget, &SnapBrowserWidget::datasetReady,
            this,             &MainWindow::onDatasetReady);
    connect(snapBrowserWidget, &SnapBrowserWidget::datasetSelected,
            this,             &MainWindow::checkRunRequirements);
    connect(runButton,        &QPushButton::clicked,
            this,             &MainWindow::onRunAlgorithmClicked);
    connect(snapBrowserWidget, &SnapBrowserWidget::analysisProgress,
            this,             &MainWindow::onAnalysisProgress);
}

void MainWindow::checkRunRequirements() {
    bool hasAlgo  = (algorithmCombo->currentIndex() > 0);
    bool hasGraph = !currentGraphPath.isEmpty() || snapBrowserWidget->hasSelection();
    runButton->setEnabled(hasAlgo && hasGraph);
}

void MainWindow::onDatasetReady(const QString& filePath) {
    currentGraphPath = filePath;
    checkRunRequirements();
    updateStatusBar("Dataset ready: " + filePath);
}

void MainWindow::onRunAlgorithmClicked() {
    QString algoId = algorithmCombo->currentData().toString();

    if (currentGraphPath.isEmpty())
        currentGraphPath = snapBrowserWidget->selectedFilePath();

    if (currentGraphPath.isEmpty()) {
        resultsText->append("Dataset not downloaded yet.");
        return;
    }

    resultsText->clear();

    if (algoId == "exact_arboricity") {
        resultsText->append("═══════════════════════════════════════════");
        resultsText->append("  EXACT ARBORICITY CALCULATION");
        resultsText->append("═══════════════════════════════════════════");
        if (snapBrowserWidget)
            snapBrowserWidget->handleAnalysis(currentGraphPath);

    } else if (algoId == "triangle_counting") {
        resultsText->append("═══════════════════════════════════════════");
        resultsText->append("  TRIANGLE COUNTING");
        resultsText->append("  (Arboricity computed automatically)");
        resultsText->append("═══════════════════════════════════════════");

        // Get SNAP triangle count for the selected dataset
        long long T_snap = snapBrowserWidget->selectedTriangleCount();
        handleTriangleCounting(currentGraphPath, T_snap);
        // Get SNAP triangle count for the selected dataset
         T_snap = snapBrowserWidget->selectedTriangleCount();
        handleTriangleCounting(currentGraphPath, T_snap);
    }
}

void MainWindow::handleTriangleCounting(const QString& filePath, long long T_snap) {
    auto* workerThread = new QThread(this);
    auto* worker       = new TriangleCounterWorker(filePath, T_snap);
    worker->moveToThread(workerThread);

    connect(workerThread, &QThread::started,
            worker,       &TriangleCounterWorker::process);
    connect(worker, &TriangleCounterWorker::finished,
            this,   &MainWindow::onTriangleCountingFinished);
    connect(worker, &TriangleCounterWorker::error,
            this,   &MainWindow::onTriangleCountingError);
    connect(worker, &TriangleCounterWorker::progress,
            this,   &MainWindow::onAnalysisProgress);

    connect(worker,       &TriangleCounterWorker::finished,
            workerThread, &QThread::quit);
    connect(workerThread, &QThread::finished,
            worker,       &QObject::deleteLater);
    connect(workerThread, &QThread::finished,
            workerThread, &QObject::deleteLater);

    workerThread->start();
}

void MainWindow::onTriangleCountingFinished(double result) {
    resultsText->append("─────────────────────────────────────");
    resultsText->append(QString("Final Count Result: %1").arg(result));
}

void MainWindow::onTriangleCountingError(const QString& message) {
    QMessageBox::critical(this, "Triangle Counting Error", message);
}

void MainWindow::onAnalysisProgress(const QString& message) {
    resultsText->append(message);
    resultsText->moveCursor(QTextCursor::End);
}

void MainWindow::setupMenuBar()       {}
void MainWindow::loadSnapDatasets()   {}
void MainWindow::handleNetworkReply() {}
void MainWindow::onGraphSelected()    {}
void MainWindow::onGraphDoubleClicked() {}
void MainWindow::updateStatusBar(const QString& message) {
    if (statusBar()) statusBar()->showMessage(message);
}
