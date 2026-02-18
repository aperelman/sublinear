#include "mainwindow.h"
#include "graph_list_widget.h"
#include "snap_browser_widget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QStatusBar>
#include <QDir>
#include <QFileInfoList>
#include <QComboBox>
#include <QPushButton>
#include <QTextEdit>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupUI();
    setupMenuBar();
    networkManager = new QNetworkAccessManager(this);
    connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::handleNetworkReply);
    loadSnapDatasets();
}

MainWindow::~MainWindow() {}

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

    // Scan the algorithms directory
    QDir dir(QDir::currentPath());
    if (dir.dirName() == "build") dir.cdUp();
    
    if (dir.cd("algorithms")) {
        dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
        QFileInfoList subDirs = dir.entryInfoList();
        for (const QFileInfo& dirInfo : subDirs) {
            algorithmCombo->addItem(dirInfo.fileName(), dirInfo.absoluteFilePath());
        }
    }
    
    algoLayout->addWidget(new QLabel("Algorithm:"));
    algoLayout->addWidget(algorithmCombo);
    
    runButton = new QPushButton("Run");
    runButton->setEnabled(false);
    algoLayout->addWidget(runButton);
    
    rightLayout->addWidget(algoGroup);
    
    resultsText = new QTextEdit();
    resultsText->setReadOnly(true);
    rightLayout->addWidget(resultsText);
    
    mainLayout->addWidget(rightPanel, 1);
    setCentralWidget(centralWidget);

    // Signal Connections
    connect(algorithmCombo, &QComboBox::currentIndexChanged, this, &MainWindow::checkRunRequirements);
    connect(snapBrowserWidget, &SnapBrowserWidget::datasetReady, this, &MainWindow::onDatasetReady);
    connect(runButton, &QPushButton::clicked, this, &MainWindow::onRunAlgorithmClicked);
}

void MainWindow::checkRunRequirements() {
    bool hasAlgo = (algorithmCombo->currentIndex() > 0);
    bool hasGraph = !currentGraphPath.isEmpty();
    runButton->setEnabled(hasAlgo && hasGraph);
}

void MainWindow::onGraphSelected() {
    // This should ideally be handled by a signal from graphListWidget
    currentGraphPath = "local_path_placeholder"; 
    checkRunRequirements();
}

void MainWindow::onDatasetReady(const QString& filePath) {
    currentGraphPath = filePath;
    checkRunRequirements();
    updateStatusBar("Dataset ready: " + filePath);
}

void MainWindow::onRunAlgorithmClicked() {
    QString selectedAlgo = algorithmCombo->currentText();
    if (selectedAlgo.contains("Exact", Qt::CaseInsensitive)) {
        if (snapBrowserWidget) {
            snapBrowserWidget->handleAnalysis(currentGraphPath); 
        }
    } else {
        resultsText->append("Running standard approximation for: " + currentGraphPath);
    }
}

void MainWindow::setupMenuBar() {}
void MainWindow::loadSnapDatasets() {}
void MainWindow::handleNetworkReply() {}
void MainWindow::onGraphDoubleClicked() {}
void MainWindow::updateStatusBar(const QString& m) { 
    if(statusBar()) statusBar()->showMessage(m); 
}