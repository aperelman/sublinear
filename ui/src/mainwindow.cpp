#include "mainwindow.h"
#include "download_manager.h"
#include "snap_catalog.h" // Ensure this defines 'SnapCatalog'
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QDateTime>
#include <QTextEdit>
#include <QComboBox>
#include <QProgressBar>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupManualLayout();
}

void MainWindow::onSyncClicked() {
    appendLog("Syncing SNAP catalog...");
    m_btnSync->setEnabled(false);
    m_progressBar->setVisible(true);
    m_progressBar->setRange(0, 0);

    auto* manager = new DownloadManager(this);

    // Use the lambda carefully to avoid memory access issues
    connect(manager, &DownloadManager::finished, this, [this, manager](const QString& path) {
        m_btnSync->setEnabled(true);
        m_progressBar->setVisible(false);

        if (!path.isEmpty()) {
            appendLog("Catalog data received. Updating list...");
            this->loadSnapMetadata(path);
        } else {
            appendLog("Sync failed: Check your internet connection.", true);
        }
        manager->deleteLater(); // Safe cleanup
    });

    manager->startDownload("https://snap.stanford.edu/data/index.html");
}

void MainWindow::loadSnapMetadata(const QString& path) {
    Q_UNUSED(path); // SNAPCatalog::get() uses hardcoded data in this version
    m_listSnap->clear();

    // 1. Fixed name to SNAPCatalog (all caps)
    // 2. Used the static get() method
    const auto& datasets = SNAPCatalog::get();

    for (const auto& ds : datasets) {
        // Create item with the dataset name
        QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(ds.name), m_listSnap);

        // Fix C2660: Pass Role AND Data. Using ds.url from the struct
        item->setData(Qt::UserRole, QString::fromStdString(ds.url));
    }

    appendLog(QString("Success: Loaded %1 datasets from SNAP catalog.").arg(datasets.size()));
}

void MainWindow::onRunClicked() { appendLog("Run clicked..."); }
void MainWindow::onLocalItemSelected(QListWidgetItem* item) { Q_UNUSED(item); }
void MainWindow::onSnapItemSelected(QListWidgetItem* item) { Q_UNUSED(item); }

void MainWindow::appendLog(const QString& message, bool isError) {
    QString time = QDateTime::currentDateTime().toString("hh:mm:ss");
    m_logArea->append(QString("[%1] %2").arg(time, message));
}

void MainWindow::setupManualLayout() {
    auto* centralWidget = new QWidget(this);
    auto* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(15);

    // --- LEFT COLUMN: DATA & INFO (Width: ~35%) ---
    auto* leftColumn = new QVBoxLayout();
    
    m_tabs = new QTabWidget(this);
    m_tabs->setMinimumWidth(280); 

    // Tabs Setup (SNAP and Local)
    auto* snapTab = new QWidget();
    auto* snapVBox = new QVBoxLayout(snapTab);
    m_btnSync = new QPushButton("Sync SNAP Catalog", this);
    m_listSnap = new QListWidget(this);
    snapVBox->addWidget(m_btnSync);
    snapVBox->addWidget(m_listSnap);
    m_tabs->addTab(snapTab, "SNAP Catalog");

    auto* localTab = new QWidget();
    auto* localVBox = new QVBoxLayout(localTab);
    m_listLocal = new QListWidget(this);
    localVBox->addWidget(new QLabel("Local Files:"));
    localVBox->addWidget(m_listLocal);
    m_tabs->addTab(localTab, "Local");

    leftColumn->addWidget(m_tabs); 

    // Metadata Panel
    auto* metaGroup = new QGroupBox("Graph Selection Info", this);
    auto* metaLayout = new QVBoxLayout(metaGroup);
    m_lblNodes = new QLabel("Nodes: --", this);
    m_lblEdges = new QLabel("Edges: --", this);
    m_lblResult = new QLabel("Result: --", this);
    metaLayout->addWidget(m_lblNodes);
    metaLayout->addWidget(m_lblEdges);
    metaLayout->addWidget(m_lblResult);
    leftColumn->addWidget(metaGroup);

    // Stretch 5 for the Left Side
    mainLayout->addLayout(leftColumn, 5); 

    // --- RIGHT COLUMN: CONFIG, RUN & LOGS (Width: ~65%) ---
    auto* rightColumn = new QVBoxLayout();

    // 1. Algorithm Selection
    auto* configGroup = new QGroupBox("Configuration", this);
    auto* configLayout = new QVBoxLayout(configGroup);
    m_comboAlgo = new QComboBox(this);
    m_comboAlgo->addItems({"Exact Arboricity", "Triangle Counting"});
    configLayout->addWidget(new QLabel("Algorithm:"));
    configLayout->addWidget(m_comboAlgo);
    rightColumn->addWidget(configGroup);

    // 2. RUN BUTTON (Shortened Height)
    auto* btnRun = new QPushButton("RUN ANALYSIS", this);
    btnRun->setMinimumHeight(35); // Decreased from 55/60 to 35
    btnRun->setMaximumHeight(40); // Ensures it stays compact
    btnRun->setStyleSheet("font-weight: bold;");
    rightColumn->addWidget(btnRun);

    // 3. System Logs
    auto* logGroup = new QGroupBox("System Log", this);
    auto* logLayout = new QVBoxLayout(logGroup);
    m_logArea = new QTextEdit(this);
    m_logArea->setReadOnly(true);
    m_logArea->setStyleSheet("background-color: #1e1e1e; color: #d4d4d4; font-family: 'Consolas';");
    logLayout->addWidget(m_logArea);
    
    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);
    logLayout->addWidget(m_progressBar);
    rightColumn->addWidget(logGroup);

    // Stretch 9 for the Right Side
    mainLayout->addLayout(rightColumn, 9); 

    setCentralWidget(centralWidget);

    // Re-connect signals
    connect(m_btnSync, &QPushButton::clicked, this, &MainWindow::onSyncClicked);
    connect(btnRun, &QPushButton::clicked, this, &MainWindow::onRunClicked);
    connect(m_listLocal, &QListWidget::itemClicked, this, &MainWindow::onLocalItemSelected);
    connect(m_listSnap, &QListWidget::itemClicked, this, &MainWindow::onSnapItemSelected);
}