#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , downloader(new GraphDownloader(this))
    , algorithmRunner(new AlgorithmRunner(this))
{
    setupUI();
    
    // Connect downloader signals
    connect(downloader, &GraphDownloader::downloadProgress,
            this, &MainWindow::onDownloadProgress);
    connect(downloader, &GraphDownloader::downloadFinished,
            this, &MainWindow::onDownloadFinished);
    connect(downloader, &GraphDownloader::extractionProgress,
            this, &MainWindow::onExtractionProgress);
    
    // Connect algorithm runner signals
    connect(algorithmRunner, &AlgorithmRunner::finished,
            this, &MainWindow::onAlgorithmFinished);
    
    // Populate graph list
    graphListWidget->populateGraphs();
}

MainWindow::~MainWindow() {
}

void MainWindow::setupUI() {
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);
    
    // Left panel: Graph list
    QGroupBox* graphGroup = new QGroupBox("Available SNAP Graphs");
    QVBoxLayout* graphLayout = new QVBoxLayout(graphGroup);
    
    graphListWidget = new GraphListWidget();
    connect(graphListWidget, &GraphListWidget::graphDownloadRequested,
            this, &MainWindow::onGraphDownloadRequested);
    connect(graphListWidget, &GraphListWidget::graphAnalyzeRequested,
            this, &MainWindow::onGraphAnalyzeRequested);
    
    graphLayout->addWidget(graphListWidget);
    
    mainLayout->addWidget(graphGroup, 1);
    
    // Right panel: Analysis controls and output
    QWidget* rightPanel = new QWidget();
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    
    // Algorithm selection
    QGroupBox* controlGroup = new QGroupBox("Analysis Controls");
    QVBoxLayout* controlLayout = new QVBoxLayout(controlGroup);
    
    QHBoxLayout* algoLayout = new QHBoxLayout();
    algoLayout->addWidget(new QLabel("Algorithm:"));
    
    algorithmSelector = new QComboBox();
    algorithmSelector->addItem("Triangle Counting");
    algorithmSelector->addItem("Large Set Arboricity");
    algorithmSelector->addItem("Degree Distribution");
    algoLayout->addWidget(algorithmSelector, 1);
    
    controlLayout->addLayout(algoLayout);
    
    runButton = new QPushButton("Run Algorithm");
    runButton->setEnabled(false);
    controlLayout->addWidget(runButton);
    
    rightLayout->addWidget(controlGroup);
    
    // Output display
    QGroupBox* outputGroup = new QGroupBox("Output");
    QVBoxLayout* outputLayout = new QVBoxLayout(outputGroup);
    
    outputDisplay = new QTextEdit();
    outputDisplay->setReadOnly(true);
    outputDisplay->setFont(QFont("Courier", 10));
    outputLayout->addWidget(outputDisplay);
    
    rightLayout->addWidget(outputGroup, 1);
    
    mainLayout->addWidget(rightPanel, 2);
    
    // Status bar
    statusBar()->showMessage("Ready. Select a graph to download or analyze.");
    
    setWindowTitle("Sublinear Graph Algorithms UI");
}

void MainWindow::onGraphDownloadRequested(const GraphInfo& graph) {
    if (downloader->isDownloading()) {
        QMessageBox::warning(this, "Download in Progress",
            "Please wait for the current download to complete.");
        return;
    }
    
    currentDownloadingGraph = QString::fromStdString(graph.name);
    outputDisplay->append(QString("Starting download: %1").arg(currentDownloadingGraph));
    statusBar()->showMessage("Downloading " + currentDownloadingGraph + "...");
    
    downloader->downloadGraph(graph);
}

void MainWindow::onGraphAnalyzeRequested(const GraphInfo& graph) {
    outputDisplay->append(QString("\n=== Analyzing: %1 ===").arg(QString::fromStdString(graph.name)));
    outputDisplay->append(QString("Graph: %1 nodes, %2 edges")
        .arg(graph.nodes).arg(graph.edges));
    outputDisplay->append(QString("Path: %1").arg(QString::fromStdString(graph.path)));
    
    runButton->setEnabled(true);
    statusBar()->showMessage("Graph selected: " + QString::fromStdString(graph.name));
}

void MainWindow::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    if (bytesTotal > 0) {
        int percentage = (bytesReceived * 100) / bytesTotal;
        statusBar()->showMessage(QString("Downloading %1: %2%")
            .arg(currentDownloadingGraph).arg(percentage));
        
        // Update the list item's progress
        for (int i = 0; i < graphListWidget->count(); ++i) {
            QListWidgetItem* item = graphListWidget->item(i);
            GraphListItemWidget* widget = qobject_cast<GraphListItemWidget*>(
                graphListWidget->itemWidget(item));
            if (widget && QString::fromStdString(widget->getGraphInfo().name) == currentDownloadingGraph) {
                widget->setDownloadProgress(percentage);
                break;
            }
        }
    }
}

void MainWindow::onExtractionProgress(int percentage) {
    statusBar()->showMessage(QString("Extracting %1: %2%")
        .arg(currentDownloadingGraph).arg(percentage));
}

void MainWindow::onDownloadFinished(bool success, const QString& errorMessage) {
    if (success) {
        outputDisplay->append(QString("✓ Download completed: %1").arg(currentDownloadingGraph));
        statusBar()->showMessage("Download completed successfully", 3000);
        
        // Update the graph list to show it's downloaded
        graphListWidget->updateGraphStatus(currentDownloadingGraph, true);
    } else {
        outputDisplay->append(QString("✗ Download failed: %1").arg(errorMessage));
        statusBar()->showMessage("Download failed", 3000);
        QMessageBox::critical(this, "Download Error", errorMessage);
    }
    
    currentDownloadingGraph.clear();
}

void MainWindow::onAlgorithmFinished(bool success, const QString& result) {
    if (success) {
        outputDisplay->append("\n" + result);
        statusBar()->showMessage("Algorithm completed", 3000);
    } else {
        outputDisplay->append("\nError: " + result);
        statusBar()->showMessage("Algorithm failed", 3000);
    }
}
