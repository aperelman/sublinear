#include "mainwindow.h"
#include "snap_browser_widget.h"
#include "local_files_widget.h"
#include "download_manager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QLocale>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    // 1. Initialize the shared DownloadManager first
    downloadManager = new DownloadManager(this);

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);

    // Left Column
    QVBoxLayout *leftColumn = new QVBoxLayout();
    tabWidget = new QTabWidget(this);

    // 2. Pass the manager to the browser widget
    // This satisfies: SnapBrowserWidget(DownloadManager *mgr, QWidget *parent)
    snapBrowser = new SnapBrowserWidget(downloadManager, this);
    localFilesTab = new LocalFilesWidget(this);

    tabWidget->addTab(localFilesTab, "Local Files");
    tabWidget->addTab(snapBrowser, "SNAP Datasets");
    leftColumn->addWidget(tabWidget, 1);

    // Property Box (Preserved exactly)
    QFrame *detailsBox = new QFrame(this);
    QVBoxLayout *boxLayout = new QVBoxLayout(detailsBox);
    label_nodes = new QLabel("Nodes: -");
    label_edges = new QLabel("Edges: -");
    label_triangles = new QLabel("Triangles: -");
    boxLayout->addWidget(label_nodes);
    boxLayout->addWidget(label_edges);
    boxLayout->addWidget(label_triangles);
    leftColumn->addWidget(detailsBox);
    mainLayout->addLayout(leftColumn, 2);

    // Right Column (Preserved exactly)
    QVBoxLayout *rightColumn = new QVBoxLayout();
    algoSelection = new QComboBox(this);
    algoSelection->addItems({"PageRank", "Connected Components"});
    btn_run = new QPushButton("Run Analysis");
    messagePanel = new QTextEdit();
    messagePanel->setReadOnly(true);
    rightColumn->addWidget(new QLabel("Algorithm:"));
    rightColumn->addWidget(algoSelection);
    rightColumn->addWidget(btn_run);
    rightColumn->addWidget(messagePanel, 1);
    mainLayout->addLayout(rightColumn, 1);

    // LOGIC CONNECTIONS
    connect(snapBrowser, &SnapBrowserWidget::datasetMetadataLoaded, this, &MainWindow::updateProperties);
    connect(snapBrowser, &SnapBrowserWidget::logMessage, this, &MainWindow::logMessage);

    // Download Requested from SNAP tab
    connect(snapBrowser, &SnapBrowserWidget::downloadRequested, this, [this](const QString &name, const QUrl &url){
        logMessage("Starting download for: " + name);
        QString savePath = "C:/BIU/data/" + name + ".txt.gz";
        downloadManager->startDownload(url, savePath);
    });

    // Refresh Local Files whenever ANY download completes
    connect(downloadManager, &DownloadManager::finished, this, [this](const QString &path){
        localFilesTab->scanDirectory("C:/BIU/data");
        logMessage("File synchronized: " + path);
    });

    connect(btn_run, &QPushButton::clicked, this, &MainWindow::handleRunClicked);

    setWindowTitle("Graph Analyzer");
    resize(1100, 750);

    // Initial directory scan
    localFilesTab->scanDirectory("C:/BIU/data");
}

void MainWindow::logMessage(const QString &msg) {
    messagePanel->append("[" + QDateTime::currentDateTime().toString("hh:mm:ss") + "] " + msg);
}

void MainWindow::updateProperties(const QString &name, int64_t nodes, int64_t edges, int64_t triangles) {
    m_currentSnapName = name;
    QLocale locale(QLocale::English);
    label_nodes->setText("Nodes: " + locale.toString((qlonglong)nodes));
    label_edges->setText("Edges: " + locale.toString((qlonglong)edges));
    label_triangles->setText("Triangles: " + locale.toString((qlonglong)triangles));
}

void MainWindow::handleRunClicked() {
    if (m_currentSnapName.isEmpty()) return;
    logMessage("Running " + algoSelection->currentText() + " on " + m_currentSnapName);
}