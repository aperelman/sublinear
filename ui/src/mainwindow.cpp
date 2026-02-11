#include "mainwindow.h"
#include "graph_list_widget.h"
#include "snap_browser_widget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QStatusBar>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    // 1. Build the UI
    setupUI();
    setupMenuBar();

    // 2. Network Setup
    networkManager = new QNetworkAccessManager(this);
    connect(networkManager, &QNetworkAccessManager::finished, 
            this, &MainWindow::handleNetworkReply);

    // 3. Load initial data
    loadSnapDatasets();
}

MainWindow::~MainWindow() {
    // No 'delete ui' needed! Qt deletes child widgets automatically.
}

void MainWindow::setupUI() {
    setWindowTitle("GraphAnalyzer - Graph Analysis Tool");
    resize(1400, 900);
    
    auto* centralWidget = new QWidget(this);
    auto* mainLayout = new QHBoxLayout(centralWidget);
    
    // Left Tab Widget setup
    leftTabWidget = new QTabWidget();
    leftTabWidget->setMinimumWidth(400);
    
    graphListWidget = new GraphListWidget();
    leftTabWidget->addTab(graphListWidget, "Local Files");
    
    snapBrowserWidget = new SnapBrowserWidget();
    leftTabWidget->addTab(snapBrowserWidget, "SNAP Datasets");
    
    mainLayout->addWidget(leftTabWidget);

    // Right Panel (Controls & Results)
    auto* rightPanel = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightPanel);
    
    auto* algoGroup = new QGroupBox("Algorithm Selection");
    auto* algoLayout = new QVBoxLayout(algoGroup);
    
    algorithmCombo = new QComboBox();
    algorithmCombo->addItem("Arboricity", "arboricity");
    algoLayout->addWidget(new QLabel("Algorithm:"));
    algoLayout->addWidget(algorithmCombo);
    
    runButton = new QPushButton("Run Analysis");
    runButton->setEnabled(false);
    connect(runButton, &QPushButton::clicked, this, &MainWindow::onRunAlgorithmClicked);
    algoLayout->addWidget(runButton);
    
    rightLayout->addWidget(algoGroup);
    
    tabWidget = new QTabWidget();
    resultsText = new QTextEdit();
    resultsText->setReadOnly(true);
    tabWidget->addTab(resultsText, "Results");
    
    rightLayout->addWidget(tabWidget);
    mainLayout->addWidget(rightPanel, 1);
    
    setCentralWidget(centralWidget);

    // Signals from custom widgets
    connect(graphListWidget, &GraphListWidget::graphSelected, this, &MainWindow::onGraphSelected);
    connect(snapBrowserWidget, &SnapBrowserWidget::datasetReady, this, &MainWindow::onDatasetReady);
}

// Implement placeholders so the linker doesn't complain
void MainWindow::setupMenuBar() {}
void MainWindow::loadSnapDatasets() {}
void MainWindow::handleNetworkReply() {}
void MainWindow::onRunAlgorithmClicked() {}
void MainWindow::onGraphSelected() { if(runButton) runButton->setEnabled(true); }
void MainWindow::onGraphDoubleClicked() {}
void MainWindow::onDatasetReady() {}
void MainWindow::updateStatusBar(const QString& m) { if(statusBar()) statusBar()->showMessage(m); }