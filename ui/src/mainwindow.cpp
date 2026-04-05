#include "mainwindow.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QTextCursor>
#include <QDateTime>
#include <QElapsedTimer>
#include <QDesktopServices>
#include <QUrl>
#include <zlib.h>
#include <QDebug>
#include <QDockWidget>
#include <QListWidget>
#include <QTextBrowser>
#include <QFileInfo>
#include <QThreadPool>
#include <vector>
#include <QMouseEvent>
#include <QMenuBar>
#include <QMenu>
#include <QToolBar>

static const char* kColorPhase   = "#5dade2";
static const char* kColorGraph   = "#6c3483";
static const char* kColorResult  = "#1e8449";
static const char* kColorWarning = "#d35400";
static const char* kColorError   = "#c0392b";
static const char* kColorGold    = "#f1c40f";

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_isAnalysisRunning(false)
    , m_downloadManager(std::make_unique<DownloadManager>())
    , m_runner(std::make_unique<AlgorithmRunner>())
{
    setWindowTitle("Graph Analysis Suite");
    resize(1200, 800);

    auto *central = new QWidget(this);
    setCentralWidget(central);
    auto *mainLayout = new QHBoxLayout(central);

    m_splitter = new QSplitter(Qt::Horizontal, this);
    mainLayout->addWidget(m_splitter);

    m_tabWidget = new QTabWidget(this);
    m_splitter->addWidget(m_tabWidget);

    // --- Local Files Tab ---
    auto *localWidget = new QWidget(this);
    auto *localLayout = new QVBoxLayout(localWidget);
    localLayout->setSpacing(4);

    auto *folderBar    = new QWidget(this);
    auto *folderLayout = new QHBoxLayout(folderBar);
    folderLayout->setContentsMargins(0, 0, 0, 0);
    folderLayout->setSpacing(4);

    m_localFolderEdit = new QLineEdit(this);
    m_localFolderEdit->setReadOnly(true);
    m_localFolderEdit->setPlaceholderText("Current folder...");

    auto *btnBrowseFolder = new QPushButton("Browse Folder...", this);
    auto *btnBrowseFile   = new QPushButton("Open File...", this);

    folderLayout->addWidget(m_localFolderEdit, 1);
    folderLayout->addWidget(btnBrowseFolder);
    folderLayout->addWidget(btnBrowseFile);
    localLayout->addWidget(folderBar);

    m_localFileView = new QTreeView(this);
    m_fileModel = new QFileSystemModel(this);
    m_fileModel->setFilter(QDir::Files | QDir::NoDotAndDotDot);
    m_fileModel->setNameFilters(QStringList() << "*.txt" << "*.txt.gz" << "*.gz");
    m_fileModel->setNameFilterDisables(false);

    QString downloadDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    m_fileModel->setRootPath(downloadDir);
    m_localFolderEdit->setText(downloadDir);

    m_localFileView->setModel(m_fileModel);
    m_localFileView->setRootIndex(m_fileModel->index(downloadDir));
    m_localFileView->setHeaderHidden(true);
    m_localFileView->setSelectionMode(QAbstractItemView::SingleSelection);
    localLayout->addWidget(m_localFileView, 1);

    m_tabWidget->addTab(localWidget, "Local Files");

    connect(btnBrowseFolder, &QPushButton::clicked, this, [this](){
        QString dir = QFileDialog::getExistingDirectory(
            this, "Select Folder Containing Graph Files",
            m_localFolderEdit->text().isEmpty()
                ? QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)
                : m_localFolderEdit->text());
        if (dir.isEmpty()) return;
        m_fileModel->setRootPath(dir);
        m_localFileView->setRootIndex(m_fileModel->index(dir));
        m_localFolderEdit->setText(dir);
        logHtml(QString("<font color='%1'>Browsing folder: %2</font>").arg(kColorPhase).arg(dir.toHtmlEscaped()));
    });

    connect(btnBrowseFile, &QPushButton::clicked, this, [this](){
        QString path = QFileDialog::getOpenFileName(
            this, "Open Graph File",
            m_localFolderEdit->text().isEmpty()
                ? QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)
                : m_localFolderEdit->text(),
            "Graph Files (*.txt *.txt.gz *.gz);;All Files (*)");
        if (path.isEmpty()) return;
        m_editFilePath->setText(path);
        QString dir = QFileInfo(path).absolutePath();
        m_fileModel->setRootPath(dir);
        m_localFileView->setRootIndex(m_fileModel->index(dir));
        m_localFolderEdit->setText(dir);
        logHtml(QString("<font color='%1'>Selected file: %2</font>").arg(kColorPhase).arg(path.toHtmlEscaped()));
    });

    // --- SNAP Datasets Tab ---
    m_snapBrowser = new SnapBrowserWidget(m_downloadManager.get(), this);
    m_tabWidget->addTab(m_snapBrowser, "SNAP Datasets");

    // --- Right Panel ---
    auto *rightPanel = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightPanel);
    m_splitter->addWidget(rightPanel);

    auto *fileGroup = new QWidget(this);
    auto *fileLayout = new QHBoxLayout(fileGroup);
    m_editFilePath = new QLineEdit(this);
    m_editFilePath->setPlaceholderText("Select a .txt graph file...");
    auto *btnBrowse = new QPushButton("Browse...", this);
    fileLayout->addWidget(new QLabel("Graph File:"));
    fileLayout->addWidget(m_editFilePath);
    fileLayout->addWidget(btnBrowse);
    rightLayout->addWidget(fileGroup);

    auto *algoGroup = new QWidget(this);
    auto *algoLayout = new QHBoxLayout(algoGroup);
    m_algoSelection = new QComboBox(this);
    m_algoSelection->addItems({
        "Exact Triangle Counting",
        "Exact Arboricity",
        "Importance Sampling Estimation"
    });

    m_degeneracyLabel = new QLabel("Degeneracy k:", this);
    m_degeneracySpinBox = new QSpinBox(this);
    m_degeneracySpinBox->setRange(1, 1000);
    m_degeneracySpinBox->setValue(50);
    m_degeneracyLabel->hide();
    m_degeneracySpinBox->hide();

    algoLayout->addWidget(new QLabel("Algorithm:"));
    algoLayout->addWidget(m_algoSelection);
    algoLayout->addWidget(m_degeneracyLabel);
    algoLayout->addWidget(m_degeneracySpinBox);
    algoLayout->addStretch();
    rightLayout->addWidget(algoGroup);

    auto *statusBox = new QWidget(this);
    auto *statusLayout = new QHBoxLayout(statusBox);
    m_labelNodes = new QLabel("Nodes: -", this);
    m_labelEdges = new QLabel("Edges: -", this);
    m_labelTriangles = new QLabel("Triangles: -", this);
    statusLayout->addWidget(m_labelNodes);
    statusLayout->addWidget(m_labelEdges);
    statusLayout->addWidget(m_labelTriangles);
    rightLayout->addWidget(statusBox);

    m_textLog = new QTextEdit(this);
    m_textLog->setReadOnly(true);
    m_textLog->setLineWrapMode(QTextEdit::NoWrap);
    m_textLog->setFontFamily("Consolas");
    m_textLog->setTextInteractionFlags(
        Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    rightLayout->addWidget(m_textLog);

    // Button row
    QWidget *buttonRow = new QWidget(this);
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonRow);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(8);

    m_labelStatus = new QLabel("Status: Ready", this);
    m_btnRun = new QPushButton("Run Analysis", this);
    m_btnRun->setMinimumHeight(40);
    m_btnRun->setStyleSheet(
        "QPushButton {"
        "  font-weight: bold;"
        "  font-size: 13px;"
        "  color: white;"
        "  background-color: #1a7a4a;"
        "  border: none;"
        "  border-radius: 4px;"
        "  padding: 6px 16px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #1e8449;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #145a32;"
        "}"
        "QPushButton:disabled {"
        "  background-color: #aab7b8;"
        "  color: #f0f0f0;"
        "}");

    QPushButton *btnReports = new QPushButton("📄 Reports", this);
    btnReports->setMinimumHeight(40);
    btnReports->setCursor(Qt::PointingHandCursor);
    btnReports->setStyleSheet(
        "QPushButton {"
        "  font-weight: bold;"
        "  font-size: 13px;"
        "  color: #2c3e50;"
        "  background-color: #ecf0f1;"
        "  border: 1px solid #bdc3c7;"
        "  border-radius: 4px;"
        "  padding: 6px 16px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #d5dbdb;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #bdc3c7;"
        "}");

    buttonLayout->addWidget(m_labelStatus, 1);
    buttonLayout->addWidget(m_btnRun);
    buttonLayout->addWidget(btnReports);

    m_btnCancelDownload = new QPushButton("✕ Cancel Download", this);
    m_btnCancelDownload->setMinimumHeight(40);
    m_btnCancelDownload->setVisible(false);
    m_btnCancelDownload->setStyleSheet(
        "QPushButton {"
        "  font-weight: bold;"
        "  font-size: 13px;"
        "  color: white;"
        "  background-color: #c0392b;"
        "  border: none;"
        "  border-radius: 4px;"
        "  padding: 6px 16px;"
        "}"
        "QPushButton:hover { background-color: #e74c3c; }"
        "QPushButton:pressed { background-color: #922b21; }");
    buttonLayout->addWidget(m_btnCancelDownload);

    rightLayout->addWidget(buttonRow);

    m_splitter->setStretchFactor(0, 9);
    m_splitter->setStretchFactor(1, 5);

    // --- Connections ---
    connect(btnBrowse, &QPushButton::clicked, [this](){
        QString path = QFileDialog::getOpenFileName(this, "Open Graph File", "", "Text Files (*.txt)");
        if (!path.isEmpty()) {
            m_editFilePath->setText(path);
            logHtml(QString("<font color='%1'>Selected local file: %2</font>").arg(kColorPhase).arg(path.toHtmlEscaped()));
        }
    });

    connect(m_btnRun, &QPushButton::clicked, this, &MainWindow::handleRunClicked);
    connect(m_btnCancelDownload, &QPushButton::clicked, this, &MainWindow::handleCancelDownload);
    connect(btnReports, &QPushButton::clicked, this, [this]() {
        if (m_reportDock) {
            m_reportDock->setVisible(!m_reportDock->isVisible());
            if (m_reportDock->isVisible()) refreshReportList();
        }
    });
    connect(m_algoSelection, &QComboBox::currentTextChanged, this, &MainWindow::onAlgoSelectionChanged);

    connect(m_localFileView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this](){
        auto indexes = m_localFileView->selectionModel()->selectedIndexes();
        if (!indexes.isEmpty()) {
            QString filePath = m_fileModel->filePath(indexes.first());
            if (filePath.endsWith(".txt")) {
                m_editFilePath->setText(filePath);
                logHtml(QString("<font color='%1'>Selected local file: %2</font>").arg(kColorPhase).arg(filePath.toHtmlEscaped()));
            }
        }
    });
    connect(m_localFileView, &QTreeView::doubleClicked, this, &MainWindow::onLocalFileSelected);

    connect(m_snapBrowser, &SnapBrowserWidget::datasetSelected, [this](const QString &name, const QUrl &url, int64_t triangles){
        m_pendingSnapName = name;
        m_pendingSnapUrl = url;
        m_pendingSnapTriangles = triangles;
        m_pendingSnapArboricity = 0.0;
        m_exactTriangleCount = 0;
        QString localPath = QDir::cleanPath(localPathForDataset(name));
        if (m_statsCache.hasDataset(localPath)) {
            DatasetStats s = m_statsCache.getDataset(localPath);
            if (s.exactTriangles > 0) {
                m_exactTriangleCount = s.exactTriangles;
                logHtml(QString("<font color='%1'>  Cached: Exact triangles = %2</font>")
                        .arg(kColorGraph).arg(s.exactTriangles));
            }
            if (s.arboricity > 0) {
                m_pendingSnapArboricity = s.arboricity;
                logHtml(QString("<font color='%1'>  Cached: Arboricity = %2</font>")
                        .arg(kColorGraph).arg(s.arboricity, 0, 'f', 4));
            }
        }
        m_editFilePath->setText(localPathForDataset(name));
        logHtml(QString("<font color='%1'><b>Selected SNAP Dataset: %2</b> (Ref Triangles: %3)</font>")
                .arg(kColorPhase).arg(name).arg(triangles));
    });

    connect(m_snapBrowser, &SnapBrowserWidget::logMessage, this, &MainWindow::handleLogMessage);
    connect(m_snapBrowser, &SnapBrowserWidget::cacheCleared, this, [this](const QString &name) {
        QString localPath = QDir::cleanPath(localPathForDataset(name));
        if (m_statsCache.hasDataset(localPath)) {
            m_statsCache.addDataset(localPath, DatasetStats{});
            m_statsCache.save(statsCachePath());
        }
        m_exactTriangleCount    = 0;
        m_pendingSnapArboricity = 0.0;
        m_lastAnalyzedPath.clear();
        m_labelTriangles->setText("Triangles: -");
    });
    connect(m_snapBrowser, &SnapBrowserWidget::cacheCleared, this, [this](const QString &name) {
        QString localPath = QDir::cleanPath(localPathForDataset(name));
        if (m_statsCache.hasDataset(localPath)) {
            m_statsCache.addDataset(localPath, DatasetStats{});
            m_statsCache.save(statsCachePath());
        }
        m_exactTriangleCount    = 0;
        m_pendingSnapArboricity = 0.0;
        m_lastAnalyzedPath.clear();
        m_labelTriangles->setText("Triangles: -");
        logHtml(QString("<font color='%1'>Stats cache cleared for: %2</font>")
                    .arg(kColorWarning).arg(name));
    });
    connect(m_runner.get(), &AlgorithmRunner::logMessage, this, &MainWindow::handleLogMessage);
    connect(m_runner.get(), &AlgorithmRunner::finished, this, &MainWindow::updateProperties);
    connect(m_runner.get(), &AlgorithmRunner::arboricityCalculated, this, &MainWindow::handleArboricityFinished);

    // Handle download errors — reset UI and suggest manual download
    connect(m_downloadManager.get(), &DownloadManager::error, this, [this](const QString &msg) {
        disconnect(m_downloadManager.get(), &DownloadManager::progress, this, nullptr);
        m_btnCancelDownload->setVisible(false);
        m_btnRun->setEnabled(true);
        m_labelStatus->setText("Status: Download Failed");
        logError(msg);
        if (!m_pendingGzPath.isEmpty()) {
            QString localPath = m_pendingGzPath;
            if (localPath.endsWith(".gz")) localPath.chop(3);
            logHtml(QString("<font color='%1'>💡 Try selecting this dataset again — the correct URL will be resolved automatically.</font>")
                        .arg(kColorWarning));
            logHtml(QString("<font color='%1'>   Or download manually and place the extracted file at: %2</font>")
                        .arg(kColorWarning).arg(localPath));
            logHtml(QString("<font color='%1'>   Then use 'Open File...' in the Local Files tab to load it.</font>")
                        .arg(kColorWarning));
        }
        m_pendingGzPath.clear();
    });
    connect(m_runner.get(), &AlgorithmRunner::arboricityFailedZero, this, [this](){
        logError("Arboricity calculation failed (result <= 0)");
        m_btnRun->setEnabled(true);
        m_labelStatus->setText("Status: Failed");
        m_isAnalysisRunning = false;
        m_pendingChainedImportanceSampling = false;
    });

    m_statsCache.load(statsCachePath());

    setupReportDock();
}

MainWindow::~MainWindow() {}

void MainWindow::onLocalFileSelected(const QModelIndex &index) {
    QString filePath = m_fileModel->filePath(index);
    if (filePath.endsWith(".txt") || filePath.endsWith(".gz")) {
        m_editFilePath->setText(filePath);
        handleRunClicked();
    }
}

void MainWindow::handleRunClicked() {
    QString path = m_editFilePath->text();
    if (path.isEmpty()) {
        logError("No graph file selected. Please choose a file from Local Files or SNAP Datasets.");
        return;
    }

    if (!QFile::exists(path)) {
        QString gzPath = path + ".gz";
        if (m_pendingSnapName.isEmpty()) {
            logError("File not found and no pending SNAP dataset. Please select a dataset from SNAP Datasets tab first.");
            return;
        }
        auto res = QMessageBox::question(this, "File Missing",
            "The dataset is not found locally. Download it now from SNAP?",
            QMessageBox::Yes | QMessageBox::No);
        if (res == QMessageBox::Yes) {
            m_btnRun->setEnabled(false);
            m_btnCancelDownload->setVisible(true);
            m_pendingGzPath = gzPath;
            m_labelStatus->setText("Status: Downloading...");
            logHtml(QString("<font color='%1'>Starting download of %2...</font>").arg(kColorPhase).arg(m_pendingSnapName));
            connect(m_downloadManager.get(), &DownloadManager::finished,
                    this, &MainWindow::handleDownloadFinished, Qt::SingleShotConnection);

            auto downloadTimer = std::make_shared<QElapsedTimer>();
            downloadTimer->start();
            disconnect(m_downloadManager.get(), &DownloadManager::progress, this, nullptr);
            connect(m_downloadManager.get(), &DownloadManager::progress,
                    this, [this, downloadTimer](qint64 bytesReceived, qint64 bytesTotal) {
                        qint64 elapsedMs = std::max(downloadTimer->elapsed(), (qint64)1);
                        double mbReceived = bytesReceived / (1024.0 * 1024.0);
                        double speedMBs   = mbReceived / (elapsedMs / 1000.0);

                        if (bytesTotal > 0) {
                            int pct = (int)(bytesReceived * 100 / bytesTotal);
                            double mbTotal = bytesTotal / (1024.0 * 1024.0);
                            qint64 remainMs = (qint64)((bytesTotal - bytesReceived) / (bytesReceived / (double)elapsedMs));
                            int remSec = (int)(remainMs / 1000);
                            QString eta = remSec > 60
                                ? QString("%1m %2s").arg(remSec / 60).arg(remSec % 60)
                                : QString("%1s").arg(remSec);
                            m_labelStatus->setText(QString("Downloading... %1%  |  %2 / %3 MB  |  %4 MB/s  |  ETA %5")
                                .arg(pct)
                                .arg(mbReceived, 0, 'f', 1)
                                .arg(mbTotal, 0, 'f', 1)
                                .arg(speedMBs, 0, 'f', 1)
                                .arg(eta));
                        } else {
                            m_labelStatus->setText(QString("Downloading... %1 MB  |  %2 MB/s")
                                .arg(mbReceived, 0, 'f', 1)
                                .arg(speedMBs, 0, 'f', 1));
                        }
                    });
            m_downloadManager->startDownload(m_pendingSnapUrl, gzPath);
        }
        return;
    }

    startAnalysis(path);
}

void MainWindow::handleDownloadFinished(const QString &gzPath) {
    m_btnCancelDownload->setVisible(false);
    m_btnRun->setEnabled(true);
    m_pendingGzPath.clear();
    disconnect(m_downloadManager.get(), &DownloadManager::progress, this, nullptr);

    if (gzPath.isEmpty() || !QFile::exists(gzPath)) {
        logError("Download failed or file not found.");
        m_labelStatus->setText("Status: Ready");
        return;
    }

    logHtml(QString("<font color='%1'>Download complete: %2</font>").arg(kColorResult).arg(QFileInfo(gzPath).fileName()));

    QString outPath = gzPath;
    if (outPath.endsWith(".gz")) outPath.chop(3);
    qint64 gzSize = QFileInfo(gzPath).size();
    m_labelStatus->setText("Status: Extracting...");
    logHtml(QString("<font color='%1'>Extracting GZip archive (%2 MB compressed)...</font>")
                .arg(kColorPhase).arg(gzSize / (1024 * 1024)));

    QThreadPool::globalInstance()->start([this, gzPath, outPath, gzSize]() {
        auto progressCb = [this, gzSize](qint64 bytesWritten) {
            // Estimate progress: bytes written / (gzSize * typical compression ratio ~3x)
            qint64 estimated = gzSize * 3;
            int pct = (int)std::min(99LL, bytesWritten * 100 / std::max(estimated, (qint64)1));
            QMetaObject::invokeMethod(this, [this, pct, bytesWritten]() {
                m_labelStatus->setText(QString("Extracting... %1 MB written (%2%)")
                    .arg(bytesWritten / (1024 * 1024)).arg(pct));
            }, Qt::QueuedConnection);
        };
        bool ok = decompressGz(gzPath, outPath, progressCb);
        QMetaObject::invokeMethod(this, [this, ok, gzPath, outPath]() {
            if (ok) {
                logHtml(QString("<font color='%1'>Extraction complete.</font>").arg(kColorResult));
                QFile::remove(gzPath);
                m_editFilePath->setText(outPath);
                if (m_snapBrowser && !m_pendingSnapName.isEmpty())
                    m_snapBrowser->markDatasetDownloaded(m_pendingSnapName);
                startAnalysis(outPath);
            } else {
                logError("Failed to extract .gz file. Check zlib installation.");
                m_btnRun->setEnabled(true);
                m_labelStatus->setText("Status: Failed");
            }
        }, Qt::QueuedConnection);
    });
}

bool MainWindow::decompressGz(const QString &gzPath, const QString &outPath,
                              std::function<void(qint64)> progressCb) {
    gzFile file = gzopen(gzPath.toLocal8Bit().constData(), "rb");
    if (!file) return false;

    constexpr int kBufSize = 1024 * 1024;  // 1MB
    gzbuffer(file, kBufSize);

    QFile outFile(outPath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        gzclose(file);
        return false;
    }

    std::vector<char> buffer(kBufSize);
    qint64 totalWritten = 0;
    qint64 lastReported = 0;
    constexpr qint64 kReportEvery = 32 * 1024 * 1024;  // every 32MB
    int len;
    while ((len = gzread(file, buffer.data(), kBufSize)) > 0) {
        outFile.write(buffer.data(), len);
        totalWritten += len;
        if (progressCb && (totalWritten - lastReported) >= kReportEvery) {
            progressCb(totalWritten);
            lastReported = totalWritten;
        }
    }
    gzclose(file);
    outFile.close();
    return (len >= 0);
}

void MainWindow::startAnalysis(const QString &filePathRaw) {
    if (m_downloadManager && m_downloadManager->isDownloading()) {
        logHtml("<font color='#d35400'><b>&#9888; Cannot start analysis — a download is still in progress.</b></font>");
        logHtml("<font color='#d35400'>Please wait for the download to complete before running analysis.</font>");
        m_labelStatus->setText("Status: Waiting for download...");
        m_btnRun->setEnabled(true);
        QMessageBox::warning(this, "Download in Progress",
            "A dataset is currently being downloaded.\n\n"
            "Please wait for the download to finish before running analysis.");
        return;
    }

    const QString filePath = QDir::cleanPath(filePathRaw);

    if (filePath.endsWith(".gz")) {
        QString outPath = filePath;
        outPath.chop(filePath.endsWith(".txt.gz") ? 7 : 3);
        outPath += ".txt";
        if (!QFile::exists(outPath)) {
            m_labelStatus->setText("Status: Extracting...");
            logHtml(QString("<font color='%1'>Extracting: %2</font>").arg(kColorPhase).arg(filePath.toHtmlEscaped()));
            QThreadPool::globalInstance()->start([this, filePath, outPath]() {
                bool ok = decompressGz(filePath, outPath, nullptr);
                QMetaObject::invokeMethod(this, [this, ok, filePath, outPath]() {
                    if (!ok) {
                        logError("Failed to extract .gz file.");
                        m_btnRun->setEnabled(true);
                        m_labelStatus->setText("Status: Failed");
                        return;
                    }
                    logHtml(QString("<font color='%1'>Extraction complete: %2</font>").arg(kColorResult).arg(outPath.toHtmlEscaped()));
                    m_editFilePath->setText(outPath);
                    startAnalysis(outPath);
                }, Qt::QueuedConnection);
            });
        } else {
            logHtml(QString("<font color='%1'>Found extracted file: %2</font>").arg(kColorPhase).arg(outPath.toHtmlEscaped()));
            m_editFilePath->setText(outPath);
            startAnalysis(outPath);
        }
        return;
    }

    m_textLog->clear();
    m_isAnalysisRunning = true;
    m_btnRun->setEnabled(false);
    m_pendingChainedImportanceSampling = false;

    logHtml(QString("<font color='%1'><b>Dataset: %2</b></font>").arg(kColorPhase).arg(QFileInfo(filePath).fileName()));

    if (filePath != m_lastAnalyzedPath) {
        m_lastAnalyzedPath      = filePath;
        m_exactTriangleCount    = 0;
        m_pendingSnapArboricity = 0.0;
        if (m_statsCache.hasDataset(filePath)) {
            DatasetStats s = m_statsCache.getDataset(filePath);
            if (s.exactTriangles > 0) {
                m_exactTriangleCount = s.exactTriangles;
                logHtml(QString("<font color='%1'>  Cache hit: Exact triangles = %2</font>")
                        .arg(kColorGraph).arg(s.exactTriangles));
            } else {
                logHtml(QString("<font color='%1'>  Cache miss: triangle count not yet computed</font>").arg(kColorGraph));
            }
            if (s.arboricity > 0) {
                m_pendingSnapArboricity = s.arboricity;
                logHtml(QString("<font color='%1'>  Cache hit: Arboricity = %2</font>")
                        .arg(kColorGraph).arg(s.arboricity, 0, 'f', 4));
            } else {
                logHtml(QString("<font color='%1'>  Cache miss: arboricity not yet computed</font>").arg(kColorGraph));
            }
        } else {
            logHtml(QString("<font color='%1'>  No cache entry for this dataset — will compute from scratch</font>").arg(kColorGraph));
        }
    } else {
        logHtml(QString("<font color='%1'>  Using in-memory values: Triangles=%2  Arboricity=%3</font>")
                .arg(kColorGraph).arg(m_exactTriangleCount).arg(m_pendingSnapArboricity, 0, 'f', 4));
    }

    QString algo = m_algoSelection->currentText();

    if (algo == "Importance Sampling Estimation") {
        bool needTriangles  = (m_exactTriangleCount <= 0);
        bool needArboricity = (m_pendingSnapArboricity <= 0.0);

        if (needTriangles) {
            m_pendingChainedImportanceSampling = true;
            m_chainedFilePath = filePath;
            logHtml(QString("<font color='%1'><b>--- Step 1/3: Computing Exact Triangle Count ---</b></font>").arg(kColorWarning));
            m_labelStatus->setText("Status: Computing Triangle Count...");
            m_runner->runTriangleCounting(filePath);
            return;
        }

        if (needArboricity) {
            m_pendingChainedImportanceSampling = true;
            m_chainedFilePath = filePath;
            logHtml(QString("<font color='%1'><b>--- Step 2/3: Computing Exact Arboricity ---</b></font>").arg(kColorWarning));
            m_labelStatus->setText("Status: Computing Arboricity...");
            m_runner->runArboricity(filePath, m_degeneracySpinBox->value());
            return;
        }

        logHtml(QString("<font color='%1'><b>--- Step 3/3: Starting Importance Sampling Estimation ---</b></font>").arg(kColorPhase));
        logHtml(QString("<font color='%1'>  Triangles=%2  Arboricity=%3</font>")
                .arg(kColorGraph).arg(m_exactTriangleCount).arg(m_pendingSnapArboricity, 0, 'f', 4));
        m_labelStatus->setText("Status: Running Importance Sampling...");
        m_runner->runImportanceSamplingEstimation(filePath, m_exactTriangleCount, m_pendingSnapArboricity);
        return;
    }

    logHtml(QString("<font color='%1'><b>--- Starting %2 ---</b></font>").arg(kColorPhase).arg(algo));
    logHtml(QString("File: %1").arg(filePath.toHtmlEscaped()));
    m_labelStatus->setText("Status: Running...");

    if (algo == "Exact Triangle Counting") {
        m_runner->runTriangleCounting(filePath);
    } else if (algo == "Exact Arboricity") {
        m_runner->runArboricity(filePath, m_degeneracySpinBox->value());
    } else if (algo == "Importance Sampling Estimation" && !m_pendingChainedImportanceSampling) {
        m_runner->runImportanceSamplingEstimation(filePath, m_pendingSnapTriangles, m_pendingSnapArboricity);
    }
}

void MainWindow::updateProperties(const QString& name, int64_t nodes, int64_t edges, int64_t triangles) {
    m_labelNodes->setText(QString("Nodes: %1").arg(nodes));
    m_labelEdges->setText(QString("Edges: %1").arg(edges));

    QString algo = m_algoSelection->currentText();
    bool isTriangleCountResult = (algo == "Exact Triangle Counting") ||
                                 (m_pendingChainedImportanceSampling && m_exactTriangleCount == 0 && triangles > 0);

    if (isTriangleCountResult) {
        m_exactTriangleCount = triangles;
        m_labelTriangles->setText(QString("Triangles: %1").arg(triangles));
        {
            DatasetStats s = m_statsCache.hasDataset(QDir::cleanPath(m_editFilePath->text()))
                ? m_statsCache.getDataset(QDir::cleanPath(m_editFilePath->text())) : DatasetStats{};
            s.exactTriangles = triangles;
            s.isValid = true;
            m_statsCache.addDataset(QDir::cleanPath(m_editFilePath->text()), s);
            m_statsCache.save(statsCachePath());
        }
        logHtml(QString("<font color='%1'><b>Exact triangle count: %2</b></font>")
                .arg(kColorResult).arg(triangles));

        if (m_pendingChainedImportanceSampling) {
            if (m_pendingSnapArboricity <= 0.0) {
                logHtml(QString("<font color='%1'><b>--- Computing Exact Arboricity (not in cache) ---</b></font>").arg(kColorWarning));
                m_labelStatus->setText("Status: Computing Arboricity...");
                m_runner->runArboricity(m_chainedFilePath, m_degeneracySpinBox->value());
            } else {
                logHtml(QString("<font color='%1'><b>--- Starting Importance Sampling Estimation ---</b></font>").arg(kColorPhase));
                logHtml(QString("<font color='%1'>  Triangles=%2  Arboricity=%3</font>")
                        .arg(kColorGraph).arg(m_exactTriangleCount).arg(m_pendingSnapArboricity, 0, 'f', 4));
                m_labelStatus->setText("Status: Running Importance Sampling...");
                m_runner->runImportanceSamplingEstimation(m_chainedFilePath, m_exactTriangleCount, m_pendingSnapArboricity);
            }
            return;
        }

        logHtml(QString("<font color='%1'><b>Analysis Complete.</b></font>").arg(kColorResult));
        saveReport();
        m_isAnalysisRunning = false;
        m_btnRun->setEnabled(true);
        m_labelStatus->setText("Status: Finished");

    } else if (algo == "Importance Sampling Estimation" && !m_pendingChainedImportanceSampling) {
        m_labelTriangles->setText(QString("Est. Triangles: %1").arg(triangles));
        int64_t exact = m_exactTriangleCount;
        if (exact > 0 && triangles >= 0) {
            double gap = std::abs(static_cast<double>(triangles - exact));
            double pct = (gap / exact) * 100.0;
            int64_t diff = triangles - exact;
            QString arrow = (diff >= 0) ? "▲" : "▼";
            QString dir   = (diff >= 0) ? "up" : "down";
            logHtml(QString("<font color='%1'><b>RESULT: Exact=%2 | Estimated=%3</b></font>")
                    .arg(kColorGold).arg(exact).arg(triangles));
            logHtml(QString("<font color='%1'><b>%2 %3 by %4 (%5%)</b></font>")
                    .arg("#d4a017").arg(arrow).arg(dir).arg(std::abs(diff)).arg(pct, 0, 'f', 2));
        }
        logHtml(QString("<font color='%1'><b>Analysis Complete.</b></font>").arg(kColorResult));
        saveReport();
        m_isAnalysisRunning = false;
        m_btnRun->setEnabled(true);
        m_labelStatus->setText("Status: Finished");

    } else if (algo == "Exact Arboricity" ||
               (algo == "Importance Sampling Estimation" && m_pendingChainedImportanceSampling)) {
        m_pendingChainedImportanceSampling = false;
        if (algo == "Exact Arboricity") {
            m_labelTriangles->setText("Triangles: -");
        }
    }
}

void MainWindow::handleArboricityFinished(double arboricity) {
    m_pendingSnapArboricity = arboricity;
    logHtml(QString("<font color='%1'><b>Arboricity Result: %2</b></font>")
            .arg(kColorResult).arg(arboricity, 0, 'f', 4));
    {
        DatasetStats s = m_statsCache.hasDataset(QDir::cleanPath(m_editFilePath->text()))
            ? m_statsCache.getDataset(QDir::cleanPath(m_editFilePath->text())) : DatasetStats{};
        s.arboricity = arboricity;
        s.isValid = true;
        m_statsCache.addDataset(QDir::cleanPath(m_editFilePath->text()), s);
        m_statsCache.save(statsCachePath());
    }

    if (m_pendingChainedImportanceSampling) {
        logHtml(QString("<font color='%1'><b>--- Starting Importance Sampling Estimation ---</b></font>").arg(kColorPhase));
        logHtml(QString("File: %1").arg(m_chainedFilePath));
        m_labelStatus->setText("Status: Running Importance Sampling...");
        m_runner->runImportanceSamplingEstimation(m_chainedFilePath, m_exactTriangleCount, m_pendingSnapArboricity);
        return;
    }

    logHtml(QString("<font color='%1'><b>Analysis Complete.</b></font>").arg(kColorResult));
    saveReport();
    m_isAnalysisRunning = false;
    m_btnRun->setEnabled(true);
    m_labelStatus->setText("Status: Finished");
}

void MainWindow::handleLogMessage(const QString &message) {
    m_textLog->append(QString("<p style='margin:0; text-decoration:none;'>%1</p>").arg(message));
}

void MainWindow::onAlgoSelectionChanged(const QString &algo) {
    bool isArb = (algo == "Exact Arboricity");
    m_degeneracyLabel->setVisible(isArb);
    m_degeneracySpinBox->setVisible(isArb);
}

void MainWindow::logHtml(const QString &html) {
    QString ts = QTime::currentTime().toString("[HH:mm:ss] ");
    // Wrap in p with explicit style to prevent Qt auto-detecting URLs as links
    m_textLog->append(QString("<p style='margin:0; text-decoration:none;'>%1%2</p>").arg(ts, html));
}

void MainWindow::logError(const QString &msg) {
    logHtml(QString("<font color='%1'>ERROR: %2</font>").arg(kColorError).arg(msg.toHtmlEscaped()));
}

QString MainWindow::statsCachePath() const {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + "/graph_stats_cache.json";
}

QString MainWindow::localPathForDataset(const QString &name) const {
    QString downloadDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    QDir().mkpath(downloadDir);
    return QDir::cleanPath(downloadDir + "/" + name + ".txt");
}

QString MainWindow::reportsFolderPath() const {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/reports";
    QDir().mkpath(dir);
    return dir;
}

void MainWindow::saveReport() {
    QString datasetName = QFileInfo(m_editFilePath->text()).baseName();
    if (datasetName.isEmpty()) datasetName = "graph";
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString algo = m_algoSelection->currentText()
                       .toLower()
                       .replace(' ', '_')
                       .replace('(', "")
                       .replace(')', "");
    QString fileName = QString("%1_%2_%3.html").arg(datasetName, algo, timestamp);
    QString filePath = reportsFolderPath() + "/" + fileName;

    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        logHtml(QString("<font color='%1'>Warning: Could not save report to %2</font>")
                    .arg(kColorWarning).arg(filePath.toHtmlEscaped()));
        return;
    }

    // Improved CSS for a browser‑like appearance
    QString html = QString(
        "<!DOCTYPE html><html><head><meta charset='utf-8'>"
        "<title>%1 — %2</title>"
        "<style>"
        "  body { "
        "    background: #ffffff; "
        "    color: #000000; "
        "    font-family: 'Segoe UI', 'Consolas', monospace; "
        "    font-size: 15px; "
        "    line-height: 1.6; "
        "    max-width: 1000px; "
        "    margin: 2rem auto; "
        "    padding: 0 20px; "
        "  }"
        "  h2 { "
        "    color: #2c3e50; "
        "    border-bottom: 2px solid #ddd; "
        "    padding-bottom: 10px; "
        "  }"
        "  p { margin: 8px 0; }"
        "  hr { border: none; border-top: 1px solid #ddd; margin: 20px 0; }"
        "  .log-line { font-family: 'Consolas', monospace; font-size: 13px; }"
        "</style>"
        "</head><body>"
        "<h2>%1</h2>"
        "<p style='color:#666;'>Algorithm: %3 &nbsp;|&nbsp; %4</p>"
        "<hr>"
        "<div class='log-line'>%5</div>"
        "</body></html>")
        .arg(datasetName)
        .arg(timestamp)
        .arg(m_algoSelection->currentText())
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"))
        .arg(m_textLog->toHtml());

    f.write(html.toUtf8());
    f.close();

    logHtml(QString("<font color='%1'>Report saved: %2</font>").arg(kColorPhase).arg(fileName));

    if (m_reportDock) {
        refreshReportList();
        if (!m_reportDock->isVisible()) {
            m_reportDock->show();
        }
    }
}

void MainWindow::setupReportDock() {
    m_reportDock = new QDockWidget("Report Viewer", this);
    m_reportDock->setObjectName("ReportViewerDock");
    m_reportDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
    m_reportDock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

    auto *container = new QWidget(this);
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    // Top bar
    auto *topBar = new QWidget(container);
    auto *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->addWidget(new QLabel("Saved Reports:", topBar));
    topLayout->addStretch();
    auto *btnRefresh = new QPushButton("Refresh", topBar);
    auto *btnOpenFolder = new QPushButton("Open Folder", topBar);
    topLayout->addWidget(btnRefresh);
    topLayout->addWidget(btnOpenFolder);
    layout->addWidget(topBar);

    // Report list
    m_reportList = new QListWidget(container);
    m_reportList->setMaximumHeight(200);
    m_reportList->setAlternatingRowColors(true);
    layout->addWidget(m_reportList);

    // HTML viewer
    m_reportBrowser = new QTextBrowser(container);
    m_reportBrowser->setOpenLinks(false);
    m_reportBrowser->setPlaceholderText("Select a report from the list above to view it here.");
    layout->addWidget(m_reportBrowser, 1);

    m_reportDock->setWidget(container);
    m_reportDock->resize(900, 600);
    m_reportDock->setFloating(true);

    addDockWidget(Qt::RightDockWidgetArea, m_reportDock);
    m_reportDock->hide();

    // Connections
    connect(m_reportDock, &QDockWidget::visibilityChanged, this, [this](bool visible) {
        if (visible) refreshReportList();
    });
    connect(btnRefresh, &QPushButton::clicked, this, &MainWindow::refreshReportList);
    connect(btnOpenFolder, &QPushButton::clicked, this, [this]() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(reportsFolderPath()));
    });
    connect(m_reportList, &QListWidget::currentItemChanged,
            this, [this](QListWidgetItem *item) {
        if (!item) return;
        QString path = item->data(Qt::UserRole).toString();
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return;
        m_reportBrowser->setHtml(QString::fromUtf8(f.readAll()));
    });
}

void MainWindow::refreshReportList() {
    if (!m_reportList) return;
    m_reportList->clear();

    QDir dir(reportsFolderPath());
    QFileInfoList files = dir.entryInfoList(
        QStringList() << "*.html",
        QDir::Files,
        QDir::Time
    );

    if (files.isEmpty()) {
        auto *placeholder = new QListWidgetItem("No reports yet.");
        placeholder->setFlags(Qt::NoItemFlags);
        m_reportList->addItem(placeholder);
        return;
    }

    for (const QFileInfo &fi : files) {
        auto *item = new QListWidgetItem(fi.fileName());
        item->setData(Qt::UserRole, fi.absoluteFilePath());
        item->setToolTip(fi.absoluteFilePath());
        m_reportList->addItem(item);
    }

    m_reportList->setCurrentRow(0);
}

void MainWindow::handleCancelDownload() {
    disconnect(m_downloadManager.get(), &DownloadManager::progress, this, nullptr);
    if (m_downloadManager && m_downloadManager->isDownloading()) {
        m_downloadManager->cancelDownload();
        if (!m_pendingGzPath.isEmpty() && QFile::exists(m_pendingGzPath)) {
            QFile::remove(m_pendingGzPath);
            logHtml(QString("<font color='%1'>Download cancelled. Partial file deleted.</font>").arg(kColorWarning));
        } else {
            logHtml(QString("<font color='%1'>Download cancelled.</font>").arg(kColorWarning));
        }
    }
    m_btnCancelDownload->setVisible(false);
    m_btnRun->setEnabled(true);
    m_labelStatus->setText("Status: Ready");
    m_pendingGzPath.clear();
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (m_isAnalysisRunning) {
        auto res = QMessageBox::warning(this, "Analysis in Progress",
            "An algorithm is currently running. Closing the application now may leave threads in an undefined state. Exit anyway?",
            QMessageBox::Yes | QMessageBox::No);
        if (res == QMessageBox::No) {
            event->ignore();
            return;
        }
    }
    event->accept();
}