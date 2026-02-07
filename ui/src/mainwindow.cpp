#include "mainwindow.h"
#include <QMenuBar>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QApplication>
#include <QFileInfo>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setupUI();
    setupMenuBar();
    
    algorithmRunner = new AlgorithmRunner(this);
    
    connect(algorithmRunner, &AlgorithmRunner::finished,
            this, &MainWindow::onAlgorithmFinished);
    connect(algorithmRunner, &AlgorithmRunner::progress,
            this, &MainWindow::onAlgorithmProgress);
    connect(algorithmRunner, &AlgorithmRunner::error,
            this, &MainWindow::onAlgorithmError);
    
    // Load SNAP datasets
    loadSnapDatasets();
    
    updateStatusBar("Ready");
}

MainWindow::~MainWindow() {
}

void MainWindow::loadSnapDatasets() {
    // Load from cache or built-in snapshot
    QVector<SnapDataset> datasets;
    
    if (SnapDatasetCache::cacheExists()) {
        datasets = SnapDatasetCache::loadFromCache();
        updateStatusBar(QString("Loaded %1 datasets from cache").arg(datasets.size()));
    } else {
        datasets = SnapDatasetCache::loadBuiltInSnapshot();
        updateStatusBar(QString("Loaded %1 datasets (built-in)").arg(datasets.size()));
    }
    
    snapBrowserWidget->setDatasets(datasets);
}

void MainWindow::setupUI() {
    setWindowTitle("GraphAnalyzer - Graph Analysis Tool");
    resize(1400, 900);
    
    auto* centralWidget = new QWidget(this);
    auto* mainLayout = new QHBoxLayout(centralWidget);
    
    // Left panel with tabs
    leftTabWidget = new QTabWidget();
    leftTabWidget->setMinimumWidth(400);
    leftTabWidget->setMaximumWidth(600);
    
    // Tab 1: Local graphs
    graphListWidget = new GraphListWidget();
    leftTabWidget->addTab(graphListWidget, "Local Files");
    
    // Tab 2: SNAP browser
    snapBrowserWidget = new SnapBrowserWidget();
    leftTabWidget->addTab(snapBrowserWidget, "SNAP Datasets");
    
    connect(graphListWidget, &GraphListWidget::graphSelected,
            this, &MainWindow::onGraphSelected);
    connect(graphListWidget, &GraphListWidget::graphDoubleClicked,
            this, &MainWindow::onGraphDoubleClicked);
    connect(snapBrowserWidget, &SnapBrowserWidget::datasetReady,
            this, &MainWindow::onDatasetReady);
    
    mainLayout->addWidget(leftTabWidget);
    
    // Right panel
    auto* rightPanel = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightPanel);
    
    auto* algoGroup = new QGroupBox("Algorithm Selection");
    auto* algoLayout = new QVBoxLayout(algoGroup);
    
    auto* algoSelectLayout = new QHBoxLayout();
    algoSelectLayout->addWidget(new QLabel("Algorithm:"));
    algorithmCombo = new QComboBox();
    algorithmCombo->addItem("Arboricity", "arboricity");
    algoSelectLayout->addWidget(algorithmCombo);
    algoLayout->addLayout(algoSelectLayout);
    
    auto* paramLayout = new QHBoxLayout();
    paramLayout->addWidget(new QLabel("Max k:"));
    maxKSpinBox = new QSpinBox();
    maxKSpinBox->setRange(1, 10000);
    maxKSpinBox->setValue(10);
    paramLayout->addWidget(maxKSpinBox);
    paramLayout->addStretch();
    algoLayout->addLayout(paramLayout);
    
    runButton = new QPushButton("Run Analysis");
    runButton->setEnabled(false);
    runButton->setStyleSheet("padding: 10px; font-weight: bold;");
    connect(runButton, &QPushButton::clicked, this, &MainWindow::onRunAlgorithmClicked);
    algoLayout->addWidget(runButton);
    
    progressBar = new QProgressBar();
    progressBar->setVisible(false);
    algoLayout->addWidget(progressBar);
    
    rightLayout->addWidget(algoGroup);
    
    tabWidget = new QTabWidget();
    resultsText = new QTextEdit();
    resultsText->setReadOnly(true);
    resultsText->setFont(QFont("Courier", 10));
    tabWidget->addTab(resultsText, "Results");
    
    rightLayout->addWidget(tabWidget);
    mainLayout->addWidget(rightPanel, 1);
    
    setCentralWidget(centralWidget);
}

void MainWindow::setupMenuBar() {
    auto* fileMenu = menuBar()->addMenu("&File");
    auto* exitAction = fileMenu->addAction("E&xit");
    connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);
    
    auto* helpMenu = menuBar()->addMenu("&Help");
    auto* aboutAction = helpMenu->addAction("&About");
    connect(aboutAction, &QAction::triggered, [this]() {
        QMessageBox::about(this, "About GraphAnalyzer",
            "<h3>GraphAnalyzer v1.0</h3>"
            "<p>Graph algorithm analysis tool</p>"
            "<p>Download and analyze SNAP datasets</p>");
    });
}

void MainWindow::onGraphSelected(const GraphInfo& graph) {
    currentGraphInfo = graph;
    runButton->setEnabled(true);
    updateStatusBar(QString("Selected: %1").arg(graph.name));
}

void MainWindow::onGraphDoubleClicked(const GraphInfo& graph) {
    currentGraphInfo = graph;
    onRunAlgorithmClicked();
}

void MainWindow::onDatasetReady(const QString& filePath) {
    // Switch to local files tab and refresh
    leftTabWidget->setCurrentIndex(0);
    
    QFileInfo fileInfo(filePath);
    graphListWidget->loadGraphsFromDirectory(fileInfo.dir().absolutePath());
    
    updateStatusBar("Dataset ready: " + fileInfo.fileName());
    
    QMessageBox::information(this, "Dataset Ready",
        QString("Dataset downloaded!\n\nFile: %1\n\nSwitch to 'Local Files' tab to select it.")
        .arg(fileInfo.fileName()));
}

void MainWindow::onRunAlgorithmClicked() {
    if (currentGraphInfo.filename.isEmpty()) {
        QMessageBox::warning(this, "No Graph", "Please select a graph first");
        return;
    }
    
    runButton->setEnabled(false);
    progressBar->setVisible(true);
    progressBar->setValue(0);
    
    resultsText->clear();
    resultsText->append(QString("Running: %1\n\n").arg(currentGraphInfo.name));
    
    updateStatusBar("Running...");
    
    QString algorithmType = algorithmCombo->currentData().toString();
    int maxK = maxKSpinBox->value();
    
    algorithmRunner->runAlgorithm(algorithmType, currentGraphInfo.filename, maxK);
}

void MainWindow::onAlgorithmFinished(const AlgorithmResult& result) {
    runButton->setEnabled(true);
    progressBar->setVisible(false);
    
    if (result.success) {
        resultsText->append("✓ Complete\n\n");
        resultsText->append(QString("Time: %1s\n\n").arg(result.executionTime, 0, 'f', 3));
        resultsText->append(result.output);
        updateStatusBar("Complete");
    } else {
        resultsText->append("✗ Failed\n\n");
        resultsText->append(result.errorMessage);
        updateStatusBar("Failed");
    }
}

void MainWindow::onAlgorithmProgress(int current, int total) {
    progressBar->setMaximum(total);
    progressBar->setValue(current);
    updateStatusBar(QString("Progress: %1/%2").arg(current).arg(total));
}

void MainWindow::onAlgorithmError(const QString& error) {
    runButton->setEnabled(true);
    progressBar->setVisible(false);
    QMessageBox::critical(this, "Error", error);
    updateStatusBar("Error");
}

void MainWindow::updateStatusBar(const QString& message) {
    statusBar()->showMessage(message);
}
