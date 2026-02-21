#include "mainwindow.h"
#include "graph_list_widget.h"
#include "snap_browser_widget.h"
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

    // Left Panel: Tabs for data sources
    leftTabWidget = new QTabWidget();
    graphListWidget = new GraphListWidget();
    snapBrowserWidget = new SnapBrowserWidget();

    leftTabWidget->addTab(graphListWidget, "Local Files");
    leftTabWidget->addTab(snapBrowserWidget, "SNAP Datasets");
    mainLayout->addWidget(leftTabWidget);

    // Right Panel: Controls and Results
    auto* rightPanel = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightPanel);

    // Algorithm Selection
    auto* algoGroup = new QGroupBox("Algorithm Selection");
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

    // Results area with styling
    resultsText = new QTextEdit();
    resultsText->setReadOnly(true);
    resultsText->setFont(QFont("Monospace", 9));
    resultsText->setStyleSheet("QTextEdit { background-color: #1e1e1e; color: #d4d4d4; }");
    rightLayout->addWidget(resultsText);

    mainLayout->addWidget(rightPanel, 1);
    setCentralWidget(centralWidget);

    // Signal Connections
    connect(algorithmCombo, &QComboBox::currentIndexChanged,
            this, &MainWindow::checkRunRequirements);
    connect(snapBrowserWidget, &SnapBrowserWidget::datasetReady,
            this, &MainWindow::onDatasetReady);
    connect(snapBrowserWidget, &SnapBrowserWidget::datasetSelected,
            this, &MainWindow::checkRunRequirements);
    connect(runButton, &QPushButton::clicked,
            this, &MainWindow::onRunAlgorithmClicked);

    // Connect progress signal from SnapBrowserWidget
    connect(snapBrowserWidget, &SnapBrowserWidget::analysisProgress,
            this, &MainWindow::onAnalysisProgress);
}

void MainWindow::checkRunRequirements() {
    bool hasAlgo  = (algorithmCombo->currentIndex() > 0);
    bool hasGraph = !currentGraphPath.isEmpty() || snapBrowserWidget->hasSelection();
    runButton->setEnabled(hasAlgo && hasGraph);
}

void MainWindow::onGraphSelected() {
    currentGraphPath = "local_path_placeholder";
    checkRunRequirements();
}

void MainWindow::onDatasetReady(const QString& filePath) {
    currentGraphPath = filePath;
    checkRunRequirements();
    updateStatusBar("Dataset ready: " + filePath);
}

void MainWindow::onRunAlgorithmClicked() {
    QString algoId = algorithmCombo->currentData().toString();

    if (algoId.isEmpty()) {
        resultsText->append("Please select an algorithm.");
        return;
    }

    // If currentGraphPath not set yet, try to get it from the widget selection
    if (currentGraphPath.isEmpty())
        currentGraphPath = snapBrowserWidget->selectedFilePath();

    if (currentGraphPath.isEmpty()) {
        resultsText->append("Dataset not downloaded yet. Please click 'Download Dataset' first.");
        return;
    }

    // Clear previous results
    resultsText->clear();

    if (algoId == "exact_arboricity") {
        resultsText->append("═══════════════════════════════════════════");
        resultsText->append("  EXACT ARBORICITY CALCULATION");
        resultsText->append("═══════════════════════════════════════════");
        resultsText->append("");

        if (snapBrowserWidget)
            snapBrowserWidget->handleAnalysis(currentGraphPath);
    } else if (algoId == "triangle_counting") {
        resultsText->append("Running Triangle Counting on: " + currentGraphPath);
        // TODO: call triangle counting logic here
    }
}

void MainWindow::onAnalysisProgress(const QString& message) {
    resultsText->append(message);
    resultsText->moveCursor(QTextCursor::End);
}

void MainWindow::setupMenuBar()    {}
void MainWindow::loadSnapDatasets() {}
void MainWindow::handleNetworkReply() {}
void MainWindow::onGraphDoubleClicked() {}

void MainWindow::updateStatusBar(const QString& message) {
    if (statusBar())
        statusBar()->showMessage(message);
}