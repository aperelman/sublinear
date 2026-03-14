#include "mainwindow.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTabWidget>
#include <QSplitter>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QStandardPaths>
#include <zlib.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_downloadManager(std::make_unique<DownloadManager>(this))
    , m_runner(std::make_unique<AlgorithmRunner>(this))
{
    auto *centralWidget = new QWidget(this);
    auto *mainLayout = new QHBoxLayout(centralWidget);
    auto *splitter = new QSplitter(Qt::Horizontal);

    // --- LEFT SIDE ---
    auto *leftSide = new QWidget();
    auto *leftLayout = new QVBoxLayout(leftSide);
    auto *tabs = new QTabWidget();

    // Tab 1: Local File
    auto *localTab = new QWidget();
    auto *localLayout = new QVBoxLayout(localTab);
    m_editFilePath = new QLineEdit();
    auto *btnBrowse = new QPushButton("Browse...");
    localLayout->addWidget(new QLabel("Local Graph File Path:"));
    localLayout->addWidget(m_editFilePath);
    localLayout->addWidget(btnBrowse);
    localLayout->addStretch();

    // Tab 2: SNAP Browser
    m_snapBrowser = new SnapBrowserWidget(m_downloadManager.get());

    tabs->addTab(localTab, "Local File");
    tabs->addTab(m_snapBrowser, "SNAP Online");
    leftLayout->addWidget(tabs);

    // --- Graph Properties ---
    auto *propsGroup = new QWidget();
    auto *propsLayout = new QVBoxLayout(propsGroup);
    m_labelNodes     = new QLabel("Nodes: —");
    m_labelEdges     = new QLabel("Edges: —");
    m_labelTriangles = new QLabel("Triangles: —");
    propsLayout->addWidget(new QLabel("<b>Graph Properties</b>"));
    propsLayout->addWidget(m_labelNodes);
    propsLayout->addWidget(m_labelEdges);
    propsLayout->addWidget(m_labelTriangles);
    leftLayout->addWidget(propsGroup);

    // --- RIGHT SIDE ---
    auto *rightSide = new QWidget();
    auto *rightLayout = new QVBoxLayout(rightSide);
    m_algoSelection = new QComboBox();
    m_algoSelection->addItems({"Exact Triangle Counting", "Arboricity Estimation"});
    m_btnRun = new QPushButton("Run Analysis");
    m_btnRun->setMinimumHeight(40);
    m_textLog = new QPlainTextEdit();
    m_textLog->setReadOnly(true);

    rightLayout->addWidget(new QLabel("Algorithm:"));
    rightLayout->addWidget(m_algoSelection);
    rightLayout->addWidget(m_btnRun);
    rightLayout->addSpacing(10);
    rightLayout->addWidget(new QLabel("Execution Log:"));
    rightLayout->addWidget(m_textLog);

    splitter->addWidget(leftSide);
    splitter->addWidget(rightSide);
    splitter->setSizes({900, 500});

    mainLayout->addWidget(splitter);
    setCentralWidget(centralWidget);
    resize(1000, 600);

    // --- SIGNALS AND SLOTS ---
    connect(btnBrowse, &QPushButton::clicked, this, [this]() {
        QString file = QFileDialog::getOpenFileName(this, "Select Graph File");
        if (!file.isEmpty()) {
            m_editFilePath->setText(file);
            m_pendingSnapName.clear();
            m_pendingSnapUrl.clear();
        }
    });

    connect(m_btnRun, &QPushButton::clicked, this, &MainWindow::handleRunClicked);

    connect(m_snapBrowser, &SnapBrowserWidget::datasetMetadataLoaded,
            this, &MainWindow::updateProperties);
    connect(m_snapBrowser, &SnapBrowserWidget::logMessage,
            m_textLog, &QPlainTextEdit::appendPlainText);

    // Track selected SNAP dataset
    connect(m_snapBrowser, &SnapBrowserWidget::datasetMetadataLoaded,
            this, [this](const QString &name, int64_t, int64_t, int64_t) {
        m_pendingSnapName = name;
        m_pendingSnapUrl  = QUrl("https://snap.stanford.edu/data/" + name + ".txt.gz");
        m_editFilePath->clear();
    });

    connect(m_runner.get(), &AlgorithmRunner::logRequest,
            m_textLog, &QPlainTextEdit::appendPlainText);
    connect(m_runner.get(), &AlgorithmRunner::finished,
            this, &MainWindow::updateProperties);
    connect(m_runner.get(), &AlgorithmRunner::arboricityFinished, this, [this](double arb) {
        m_textLog->appendPlainText(QString("Arboricity: %1").arg(arb));
    });
    connect(m_runner.get(), &AlgorithmRunner::arboricityFailedZero, this, [this]() {
        m_textLog->appendPlainText("Arboricity computation returned zero.");
    });
}

MainWindow::~MainWindow() {}

bool MainWindow::decompressGz(const QString &gzPath, const QString &outPath) {
    gzFile in = gzopen(gzPath.toLocal8Bit().constData(), "rb");
    if (!in) return false;
    QFile out(outPath);
    if (!out.open(QIODevice::WriteOnly)) {
        gzclose(in);
        return false;
    }
    char buf[65536];
    int bytesRead;
    while ((bytesRead = gzread(in, buf, sizeof(buf))) > 0)
        out.write(buf, bytesRead);
    gzclose(in);
    return true;
}

QString MainWindow::localPathForDataset(const QString &name) const {
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    return dataDir + "/" + name + ".txt";
}

void MainWindow::runAlgorithmOnFile(const QString &filePath) {
    if (m_algoSelection->currentText() == "Exact Triangle Counting") {
        m_runner->runTriangleCounting(filePath);
    } else {
        m_runner->runArboricity(filePath, 0);
    }
}

void MainWindow::handleRunClicked() {
    m_textLog->clear();

    // Case 1: local file
    QString localPath = m_editFilePath->text();
    if (!localPath.isEmpty()) {
        if (!QFile::exists(localPath)) {
            QMessageBox::warning(this, "Error", "File not found: " + localPath);
            return;
        }
        m_textLog->appendPlainText("Initializing analysis on local file...");
        runAlgorithmOnFile(localPath);
        return;
    }

    // Case 2: SNAP dataset
    if (m_pendingSnapName.isEmpty()) {
        QMessageBox::warning(this, "Error", "No file or dataset selected.");
        return;
    }

    // Check cache
    QString txtPath = localPathForDataset(m_pendingSnapName);
    if (QFile::exists(txtPath)) {
        m_textLog->appendPlainText("Using cached file: " + txtPath);
        runAlgorithmOnFile(txtPath);
        return;
    }

    // Download then decompress then run
    m_textLog->appendPlainText("Downloading " + m_pendingSnapName + "...");
    m_btnRun->setEnabled(false);

    QString gzPath = txtPath + ".gz";

    connect(m_downloadManager.get(), &DownloadManager::finished,
            this, [this, gzPath, txtPath](const QString &path) {
        if (path != gzPath) return;
        m_btnRun->setEnabled(true);
        m_textLog->appendPlainText("Decompressing...");
        if (decompressGz(gzPath, txtPath)) {
            QFile::remove(gzPath);
            m_textLog->appendPlainText("Running analysis...");
            runAlgorithmOnFile(txtPath);
        } else {
            m_textLog->appendPlainText("Failed to decompress file.");
        }
    }, Qt::SingleShotConnection);

    connect(m_downloadManager.get(), &DownloadManager::error,
            this, [this](const QString &msg) {
        m_btnRun->setEnabled(true);
        m_textLog->appendPlainText("Download error: " + msg);
    }, Qt::SingleShotConnection);

    m_downloadManager->startDownload(m_pendingSnapUrl, gzPath);
}

void MainWindow::updateProperties(const QString& name, int64_t nodes, int64_t edges, int64_t triangles) {
    m_labelNodes->setText(QString("Nodes: %1").arg(nodes));
    m_labelEdges->setText(QString("Edges: %1").arg(edges));
    m_labelTriangles->setText(QString("Triangles: %1").arg(triangles));
    m_textLog->appendPlainText(QString("Displaying details for: %1").arg(name));
}
