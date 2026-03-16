#include "mainwindow.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTabWidget>
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

// ── Color palette ─────────────────────────────────────────────────────────────
static const char* kColorPhase   = "#1a5276";  // dark blue   — phase headers
static const char* kColorGraph   = "#6c3483";  // purple      — graph load info
static const char* kColorResult  = "#1e8449";  // dark green  — results
static const char* kColorWarning = "#d35400";  // orange      — warnings
static const char* kColorError   = "#c0392b";  // red         — errors

// ── Report helpers ────────────────────────────────────────────────────────────
static QString reportFilePath() {
    QString dir = QDir::homePath() + "/GraphAnalyzer/reports";
    QDir().mkpath(dir);
    return dir + "/report.html";
}

static void appendToReport(const QString &html) {
    QFile f(reportFilePath());
    bool isNew = !f.exists();
    if (!f.open(QIODevice::Append | QIODevice::Text)) return;
    QTextStream out(&f);
    if (isNew) {
        out << "<!DOCTYPE html>\n<html>\n<head>\n"
            << "<meta charset='utf-8'>\n"
            << "<title>GraphAnalyzer Report</title>\n"
            << "<style>\n"
            << "  body { font-family: monospace; font-size: 13px; margin: 20px; background: #fafafa; }\n"
            << "  .run { border: 1px solid #ccc; border-radius: 6px; padding: 16px; "
               "margin-bottom: 24px; background: white; }\n"
            << "  .run-header { font-size: 15px; font-weight: bold; color: #2c3e50; "
               "border-bottom: 1px solid #eee; padding-bottom: 8px; margin-bottom: 12px; }\n"
            << "  .timestamp { color: #888; font-size: 11px; }\n"
            << "</style>\n</head>\n<body>\n"
            << "<h1 style='color:#2c3e50;'>GraphAnalyzer Report</h1>\n";
    }
    out << html;
    f.close();
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_downloadManager(std::make_unique<DownloadManager>(this))
    , m_runner(std::make_unique<AlgorithmRunner>(this))
{
    auto *centralWidget = new QWidget(this);
    auto *mainLayout = new QHBoxLayout(centralWidget);
    auto *splitter = new QSplitter(Qt::Horizontal);

    // ------------------------------------------------------------------ LEFT
    auto *leftSide = new QWidget();
    auto *leftLayout = new QVBoxLayout(leftSide);
    auto *tabs = new QTabWidget();

    // Style the tabs with larger font
    tabs->setStyleSheet(R"(
        QTabWidget::pane {
            border: 1px solid #cccccc;
            border-radius: 4px;
            padding: 8px;
            background: white;
        }
        QTabBar::tab {
            background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                        stop: 0 #f6f7f9, stop: 1 #e6e9ef);
            border: 1px solid #cccccc;
            border-bottom: none;
            border-top-left-radius: 4px;
            border-top-right-radius: 4px;
            padding: 10px 20px;
            margin-right: 2px;
            font-weight: bold;
            font-size: 13px;
        }
        QTabBar::tab:selected {
            background: white;
            border-bottom-color: white;
            margin-bottom: -1px;
        }
        QTabBar::tab:hover:!selected {
            background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                        stop: 0 #ffffff, stop: 1 #f0f3f9);
        }
    )");

    // Local File Tab with improved layout
    auto *localTab = new QWidget();
    auto *localLayout = new QVBoxLayout(localTab);
    localLayout->setSpacing(12);
    localLayout->setContentsMargins(15, 15, 15, 15);

    // Styled section label with larger font
    auto *localLabel = new QLabel("📂 Local Graph File");
    localLabel->setStyleSheet(
        "font-weight: bold;"
        "font-size: 14px;"
        "color: #2c3e50;"
        "padding: 6px 0;"
    );

    // Horizontal layout for file input and browse button
    auto *fileInputLayout = new QHBoxLayout();
    fileInputLayout->setSpacing(10);

    m_editFilePath = new QLineEdit();
    m_editFilePath->setPlaceholderText("Select a graph file...");
    m_editFilePath->setMinimumHeight(38);
    m_editFilePath->setStyleSheet(
        "QLineEdit {"
        "   border: 1px solid #cccccc;"
        "   border-radius: 5px;"
        "   padding: 6px 10px;"
        "   background: white;"
        "   font-size: 13px;"
        "}"
        "QLineEdit:focus {"
        "   border: 2px solid #3498db;"
        "}"
    );

    auto *btnBrowse = new QPushButton("📁 Browse...");
    btnBrowse->setMinimumHeight(38);
    btnBrowse->setMinimumWidth(110);
    btnBrowse->setCursor(Qt::PointingHandCursor);
    btnBrowse->setStyleSheet(R"(
        QPushButton {
            background-color: #3498db;
            color: white;
            border: none;
            border-radius: 5px;
            padding: 6px 16px;
            font-weight: bold;
            font-size: 13px;
        }
        QPushButton:hover {
            background-color: #2980b9;
        }
        QPushButton:pressed {
            background-color: #216795;
        }
    )");

    fileInputLayout->addWidget(m_editFilePath, 3);
    fileInputLayout->addWidget(btnBrowse, 1);

    localLayout->addWidget(localLabel);
    localLayout->addLayout(fileInputLayout);
    localLayout->addStretch();

    m_snapBrowser = new SnapBrowserWidget(m_downloadManager.get());
    tabs->addTab(localTab, "Local File");
    tabs->addTab(m_snapBrowser, "SNAP Online");
    leftLayout->addWidget(tabs);

    // Graph Properties section with larger fonts
    auto *propsGroup = new QWidget();
    auto *propsLayout = new QVBoxLayout(propsGroup);
    propsLayout->setSpacing(8);
    
    m_labelNodes     = new QLabel("Nodes: —");
    m_labelEdges     = new QLabel("Edges: —");
    m_labelTriangles = new QLabel("Triangles: —");
    
    // Style the properties labels with larger font
    QString propLabelStyle = "font-weight: bold; color: #2c3e50; font-size: 14px; padding: 4px 0;";
    m_labelNodes->setStyleSheet(propLabelStyle);
    m_labelEdges->setStyleSheet(propLabelStyle);
    m_labelTriangles->setStyleSheet(propLabelStyle);
    
    auto *propsTitle = new QLabel("📊 Graph Properties");
    propsTitle->setStyleSheet("font-weight: bold; font-size: 15px; color: #34495e; padding: 6px 0;");
    
    propsLayout->addWidget(propsTitle);
    propsLayout->addWidget(m_labelNodes);
    propsLayout->addWidget(m_labelEdges);
    propsLayout->addWidget(m_labelTriangles);
    propsLayout->addStretch();
    
    // Add a subtle background to properties
    propsGroup->setStyleSheet(
        "QWidget {"
        "   background-color: #f8f9fa;"
        "   border: 1px solid #e9ecef;"
        "   border-radius: 5px;"
        "   padding: 12px;"
        "}"
    );
    
    leftLayout->addWidget(propsGroup);

    // ----------------------------------------------------------------- RIGHT
    auto *rightSide = new QWidget();
    auto *rightLayout = new QVBoxLayout(rightSide);
    rightLayout->setSpacing(12);

    // Algorithm Selection with styled label
    auto *algoLabel = new QLabel("🎯 Algorithm:");
    algoLabel->setStyleSheet("font-weight: bold; color: #2c3e50; font-size: 14px;");
    
    m_algoSelection = new QComboBox();
    m_algoSelection->addItems({
        "Exact Arboricity",
        "Exact Triangle Counting",
        "Importance Sampling (Triangle Estimation)"
    });
    m_algoSelection->setMinimumHeight(38);
    m_algoSelection->setStyleSheet(
        "QComboBox {"
        "   border: 1px solid #cccccc;"
        "   border-radius: 5px;"
        "   padding: 6px 10px;"
        "   background: white;"
        "   min-width: 280px;"
        "   font-size: 13px;"
        "}"
        "QComboBox:hover {"
        "   border: 1px solid #999999;"
        "}"
        "QComboBox::drop-down {"
        "   border: none;"
        "   width: 28px;"
        "}"
        "QComboBox::down-arrow {"
        "   image: none;"
        "   border-left: 5px solid transparent;"
        "   border-right: 5px solid transparent;"
        "   border-top: 5px solid #666;"
        "   margin-right: 10px;"
        "}"
    );

    // Degeneracy controls in a horizontal layout
    auto *degenWidget = new QWidget();
    auto *degenLayout = new QHBoxLayout(degenWidget);
    degenLayout->setContentsMargins(0, 0, 0, 0);
    degenLayout->setSpacing(10);
    
    m_degeneracyLabel = new QLabel("Degeneracy (0 = auto):");
    m_degeneracyLabel->setStyleSheet("font-size: 13px;");
    
    m_degeneracySpinBox = new QSpinBox();
    m_degeneracySpinBox->setRange(0, 100000);
    m_degeneracySpinBox->setValue(0);
    m_degeneracySpinBox->setMinimumHeight(34);
    m_degeneracySpinBox->setMinimumWidth(90);
    m_degeneracySpinBox->setToolTip(
        "Degeneracy hint for arboricity computation.\n"
        "Set to 0 to let the algorithm compute it automatically."
    );
    m_degeneracySpinBox->setStyleSheet(
        "QSpinBox {"
        "   border: 1px solid #cccccc;"
        "   border-radius: 4px;"
        "   padding: 4px 6px;"
        "   font-size: 13px;"
        "}"
    );
    
    degenLayout->addWidget(m_degeneracyLabel);
    degenLayout->addWidget(m_degeneracySpinBox);
    degenLayout->addStretch();
    
    m_degeneracyLabel->setVisible(true);
    m_degeneracySpinBox->setVisible(true);

    // Status label with improved styling and larger font
    m_labelStatus = new QLabel("α: —   T_ref: —");
    m_labelStatus->setStyleSheet(
        "QLabel {"
        "   color: #2c3e50;"
        "   font-size: 14px;"
        "   font-family: monospace;"
        "   background: #f8f9fa;"
        "   border: 1px solid #e9ecef;"
        "   border-radius: 5px;"
        "   padding: 10px;"
        "   margin: 6px 0;"
        "}"
    );

    // Run button with play icon and improved styling
    m_btnRun = new QPushButton("▶ Run Analysis");
    m_btnRun->setMinimumHeight(48);
    m_btnRun->setCursor(Qt::PointingHandCursor);
    m_btnRun->setStyleSheet(R"(
        QPushButton {
            background-color: #27ae60;
            color: white;
            border: none;
            border-radius: 5px;
            font-size: 15px;
            font-weight: bold;
            padding: 10px;
            margin: 10px 0;
        }
        QPushButton:hover {
            background-color: #2ecc71;
        }
        QPushButton:pressed {
            background-color: #229954;
        }
        QPushButton:disabled {
            background-color: #95a5a6;
        }
    )");

    // Log area with improved styling - LIGHT THEME and LARGER FONT
    auto *logLabel = new QLabel("📋 Execution Log:");
    logLabel->setStyleSheet("font-weight: bold; color: #2c3e50; margin-top: 10px; font-size: 14px;");
    
    m_textLog = new QTextEdit();
    m_textLog->setReadOnly(true);
    m_textLog->setAcceptRichText(true);
    m_textLog->setFont(QFont("Consolas", 11));
    m_textLog->setStyleSheet(
        "QTextEdit {"
        "   border: 1px solid #cccccc;"
        "   border-radius: 5px;"
        "   background-color: #ffffff;"
        "   color: #000000;"
        "   font-family: 'Consolas', 'Monaco', monospace;"
        "   font-size: 11px;"
        "   padding: 6px;"
        "   line-height: 1.4;"
        "}"
    );

    // Add widgets to right layout
    rightLayout->addWidget(algoLabel);
    rightLayout->addWidget(m_algoSelection);
    rightLayout->addWidget(degenWidget);
    rightLayout->addWidget(m_labelStatus);
    rightLayout->addWidget(m_btnRun);
    rightLayout->addSpacing(8);
    rightLayout->addWidget(logLabel);
    rightLayout->addWidget(m_textLog);

    // Add stretch to push everything up
    rightLayout->setStretchFactor(m_textLog, 2);

    splitter->addWidget(leftSide);
    splitter->addWidget(rightSide);
    splitter->setSizes({950, 550});
    
    // Style the splitter handle
    splitter->setStyleSheet(R"(
        QSplitter::handle {
            background-color: #cccccc;
            width: 3px;
        }
        QSplitter::handle:hover {
            background-color: #999999;
        }
    )");

    // Add padding and spacing to main layout
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(12);

    mainLayout->addWidget(splitter);
    setCentralWidget(centralWidget);
    
    // Set window title and size
    setWindowTitle("📊 Graph Analyzer - Sublinear Algorithms");
    resize(1300, 750);

    // --------------------------------------------------------- SIGNALS/SLOTS
    connect(btnBrowse, &QPushButton::clicked, this, [this]() {
        QString file = QFileDialog::getOpenFileName(this, "Select Graph File");
        if (!file.isEmpty()) {
            m_editFilePath->setText(file);
            m_pendingSnapName.clear();
            m_pendingSnapUrl.clear();
            m_pendingSnapTriangles = 0;
        }
    });

    connect(m_algoSelection, &QComboBox::currentTextChanged,
            this, &MainWindow::onAlgoSelectionChanged);
    connect(m_btnRun, &QPushButton::clicked, this, &MainWindow::handleRunClicked);

    connect(m_snapBrowser, &SnapBrowserWidget::datasetMetadataLoaded,
            this, &MainWindow::updateProperties);
    connect(m_snapBrowser, &SnapBrowserWidget::logMessage,
            this, &MainWindow::handleLogMessage);
    connect(m_snapBrowser, &SnapBrowserWidget::datasetMetadataLoaded,
            this, [this](const QString &name, int64_t, int64_t, int64_t triangles) {
        m_pendingSnapName      = name;
        m_pendingSnapUrl       = QUrl("https://snap.stanford.edu/data/" + name + ".txt.gz");
        if (m_pendingSnapTriangles <= 0 && triangles > 0)
            m_pendingSnapTriangles = triangles;
        m_editFilePath->clear();
    });

    connect(m_runner.get(), &AlgorithmRunner::logRequest,
            this, &MainWindow::handleLogMessage);
    connect(m_runner.get(), &AlgorithmRunner::finished,
            this, &MainWindow::updateProperties);

    connect(m_runner.get(), &AlgorithmRunner::arboricityFinished, this, [this](double arb) {
        m_pendingSnapArboricity = arb;
        logResult(QString("Arboricity: %1").arg(arb));
        updateStatusLabel();
        if (!m_pipelineFilePath.isEmpty()) {
            m_pipelineNeedTriangles = true;
            logInfo("Step 2/3: Running exact triangle counting for T_ref...");
            m_runner->runTriangleCounting(m_pipelineFilePath);
        }
    });

    connect(m_runner.get(), &AlgorithmRunner::arboricityFailedZero, this, [this]() {
        logError("Arboricity computation returned zero — pipeline aborted.");
        m_pipelineFilePath.clear();
        m_pipelineNeedTriangles = false;
    });
}

MainWindow::~MainWindow() {}

// ── Colored log helpers ───────────────────────────────────────────────────────

void MainWindow::logHtml(const QString &html) {
    m_textLog->append(html);
}

void MainWindow::logPhase(const QString &msg) {
    logHtml(QString("<span style='color:%1;font-weight:bold;font-size:11pt;'>%2</span>")
        .arg(kColorPhase).arg(msg.toHtmlEscaped()));
}

void MainWindow::logInfo(const QString &msg) {
    logHtml(QString("<span style='color:black;font-size:11pt;'>%1</span>")
        .arg(msg.toHtmlEscaped()));
}

void MainWindow::logGraph(const QString &msg) {
    logHtml(QString("<span style='color:%1;font-weight:bold;font-size:11pt;'>%2</span>")
        .arg(kColorGraph).arg(msg.toHtmlEscaped()));
}

void MainWindow::logResult(const QString &msg) {
    logHtml(QString("<span style='color:%1;font-weight:bold;font-size:11pt;'>%2</span>")
        .arg(kColorResult).arg(msg.toHtmlEscaped()));
}

void MainWindow::logWarning(const QString &msg) {
    logHtml(QString("<span style='color:%1;font-weight:bold;font-size:11pt;'>%2</span>")
        .arg(kColorWarning).arg(msg.toHtmlEscaped()));
}

void MainWindow::logError(const QString &msg) {
    logHtml(QString("<span style='color:%1;font-weight:bold;font-size:11pt;'>%2</span>")
        .arg(kColorError).arg(msg.toHtmlEscaped()));
}

void MainWindow::handleLogMessage(const QString &message) {
    if (message.startsWith("ERROR:") || message.startsWith("ABORT:"))
        logError(message);
    else if (message.startsWith("WARNING:"))
        logWarning(message);
    else if (message.startsWith("---") || message.startsWith("==="))
        logPhase(message);
    else if (message.startsWith("RESULT"))
        logResult(message);
    else if (message.startsWith("Loaded ") || message.startsWith("Graph:"))
        logGraph(message);
    else
        logInfo(message);
}

// ── Other methods ─────────────────────────────────────────────────────────────

QString MainWindow::reportsFilePath() {
    QString dir = QDir::homePath() + "/GraphAnalyzer/reports";
    QDir().mkpath(dir);
    return dir + "/report.html";
}

void MainWindow::appendReport(const QString &graphName, const QString &algo,
                               int64_t nodes, int64_t edges,
                               int64_t exactTriangles, int64_t estimatedTriangles,
                               double arboricity, double elapsedMs) {
    QString path = reportsFilePath();
    QFile f(path);
    bool isNew = !f.exists();
    if (!f.open(QIODevice::Append | QIODevice::Text)) return;
    QTextStream out(&f);

    // Write HTML header once when file is first created
    if (isNew) {
        out << "<!DOCTYPE html>\n<html>\n<head>\n"
            << "<meta charset='utf-8'>\n"
            << "<title>GraphAnalyzer Report</title>\n"
            << "<style>\n"
            << "body{font-family:monospace;font-size:13px;margin:24px;background:#f5f5f5;}\n"
            << "h1{color:#2c3e50;}\n"
            << ".run{border:1px solid #ccc;border-radius:6px;padding:16px;"
               "margin-bottom:20px;background:white;box-shadow:1px 1px 4px #ddd;}\n"
            << ".run-header{font-size:14px;font-weight:bold;color:#2c3e50;"
               "border-bottom:1px solid #eee;padding-bottom:8px;margin-bottom:10px;}\n"
            << ".ts{color:#888;font-size:11px;float:right;}\n"
            << ".label{color:#555;width:180px;display:inline-block;}\n"
            << ".val{font-weight:bold;}\n"
            << ".exact{color:#1e8449;}\n"
            << ".est{color:#1a5276;}\n"
            << ".gap-over{color:#d35400;}\n"
            << ".gap-under{color:#1a5276;}\n"
            << ".arb{color:#6c3483;}\n"
            << "</style>\n</head>\n<body>\n"
            << "<h1>GraphAnalyzer Report</h1>\n";
    }

    QString ts = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    QString elapsed = QString::number(elapsedMs / 1000.0, 'f', 2) + "s";

    out << "<div class='run'>\n";
    out << "<div class='run-header'>" << graphName.toHtmlEscaped()
        << " — " << algo.toHtmlEscaped()
        << "<span class='ts'>" << ts << "</span></div>\n";

    // Graph properties
    out << "<div><span class='label'>Nodes (n):</span>"
        << "<span class='val'>" << nodes << "</span></div>\n";
    out << "<div><span class='label'>Edges (m):</span>"
        << "<span class='val'>" << edges << "</span></div>\n";

    if (arboricity > 0)
        out << "<div><span class='label'>Arboricity (α):</span>"
            << "<span class='val arb'>" << QString::number(arboricity, 'f', 1)
            << "</span></div>\n";

    if (exactTriangles > 0)
        out << "<div><span class='label'>Exact triangles:</span>"
            << "<span class='val exact'>" << exactTriangles << "</span></div>\n";

    if (estimatedTriangles > 0) {
        out << "<div><span class='label'>Estimated triangles:</span>"
            << "<span class='val est'>" << estimatedTriangles << "</span></div>\n";

        if (exactTriangles > 0) {
            double gap = estimatedTriangles - exactTriangles;
            double pct = (gap / exactTriangles) * 100.0;
            QString pctStr = (pct >= 0 ? "+" : "") + QString::number(pct, 'f', 1) + "%";
            QString dir = (gap >= 0) ? "▲ over" : "▼ under";
            QString cls = (gap >= 0) ? "gap-over" : "gap-under";
            out << "<div><span class='label'>Gap:</span>"
                << "<span class='val " << cls << "'>"
                << dir << " by " << QString::number(std::abs(gap), 'f', 0)
                << " (" << pctStr << ")"
                << "</span></div>\n";
        }
    }

    out << "<div><span class='label'>Total elapsed:</span>"
        << "<span class='val'>" << elapsed << "</span></div>\n";
    out << "</div>\n";
    f.close();

    logInfo(QString("Report saved → %1").arg(path));
}

void MainWindow::onAlgoSelectionChanged(const QString &algo) {
    bool showDegen = (algo == "Exact Arboricity");
    m_degeneracyLabel->setVisible(showDegen);
    m_degeneracySpinBox->setVisible(showDegen);
}

bool MainWindow::decompressGz(const QString &gzPath, const QString &outPath) {
    gzFile in = gzopen(gzPath.toLocal8Bit().constData(), "rb");
    if (!in) return false;
    QFile out(outPath);
    if (!out.open(QIODevice::WriteOnly)) { gzclose(in); return false; }
    char buf[65536];
    int bytesRead;
    while ((bytesRead = gzread(in, buf, sizeof(buf))) > 0)
        out.write(buf, bytesRead);
    gzclose(in);
    return true;
}

// ---------- FIXED: Use GraphAnalyzer subfolder so files appear in Local Files tab ----------
QString MainWindow::localPathForDataset(const QString &name) const {
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)
                      + "/GraphAnalyzer";
    QDir().mkpath(dataDir);               // create folder if it doesn't exist
    return dataDir + "/" + name + ".txt";
}
// -----------------------------------------------------------------------------------------

void MainWindow::runAlgorithmOnFile(const QString &filePath) {
    const QString algo = m_algoSelection->currentText();

    if (algo == "Exact Triangle Counting") {
        m_runner->runTriangleCounting(filePath);

    } else if (algo == "Importance Sampling (Triangle Estimation)") {
        // Full 3-step pipeline: Arboricity → Exact Counting → Importance Sampling
        m_pipelineFilePath = filePath;
        m_pipelineNeedTriangles = false;

        if (m_pendingSnapArboricity <= 0) {
            logInfo("Step 1/3: Running exact arboricity...");
            m_runner->runArboricity(filePath, m_degeneracySpinBox->value());
        } else if (m_pendingSnapTriangles <= 0) {
            logInfo("Arboricity already computed. Step 2/3: Running exact triangle counting...");
            m_pipelineNeedTriangles = true;
            m_runner->runTriangleCounting(filePath);
        } else {
            logInfo("Arboricity and T_ref ready. Running importance sampling...");
            m_pipelineFilePath.clear();
            m_runner->runImportanceSamplingEstimation(
                filePath, m_pendingSnapTriangles, m_pendingSnapArboricity);
        }

    } else { // Exact Arboricity (manual)
        m_pipelineFilePath.clear();
        m_pipelineNeedTriangles = false;
        m_runner->runArboricity(filePath, m_degeneracySpinBox->value());
    }
}

void MainWindow::handleRunClicked() {
    m_textLog->clear();
    m_runStartMs = QDateTime::currentMSecsSinceEpoch();
    m_lastExactTriangles = 0;
    m_lastEstimatedTriangles = 0;

    QString localPath = m_editFilePath->text();
    if (!localPath.isEmpty()) {
        if (!QFile::exists(localPath)) {
            QMessageBox::warning(this, "Error", "File not found: " + localPath);
            return;
        }
        logInfo("Initializing analysis on local file...");
        runAlgorithmOnFile(localPath);
        return;
    }

    if (m_pendingSnapName.isEmpty()) {
        QMessageBox::warning(this, "Error", "No file or dataset selected.");
        return;
    }

    QString txtPath = localPathForDataset(m_pendingSnapName);
    if (QFile::exists(txtPath)) {
        logInfo("Using cached file: " + txtPath);
        runAlgorithmOnFile(txtPath);
        return;
    }

    logInfo("Downloading " + m_pendingSnapName + "...");
    m_btnRun->setEnabled(false);
    QString gzPath = txtPath + ".gz";

    connect(m_downloadManager.get(), &DownloadManager::finished,
            this, [this, gzPath, txtPath](const QString &path) {
        if (path != gzPath) return;
        m_btnRun->setEnabled(true);
        logInfo("Decompressing...");
        if (decompressGz(gzPath, txtPath)) {
            QFile::remove(gzPath);
            logInfo("Running analysis...");
            runAlgorithmOnFile(txtPath);
        } else {
            logError("Failed to decompress file.");
        }
    }, Qt::SingleShotConnection);

    connect(m_downloadManager.get(), &DownloadManager::error,
            this, [this](const QString &msg) {
        m_btnRun->setEnabled(true);
        logError("Download error: " + msg);
    }, Qt::SingleShotConnection);

    m_downloadManager->startDownload(m_pendingSnapUrl, gzPath);
}

void MainWindow::updateStatusLabel() {
    QString arbStr = (m_pendingSnapArboricity > 0)
        ? QString::number(m_pendingSnapArboricity, 'f', 1)
        : "— (run Exact Arboricity)";
    QString tStr = (m_pendingSnapTriangles > 0)
        ? QString::number(m_pendingSnapTriangles)
        : "—";
    m_labelStatus->setText(QString("α: %1   T_ref: %2").arg(arbStr).arg(tStr));
}

void MainWindow::updateProperties(const QString& name, int64_t nodes, int64_t edges, int64_t triangles) {
    // nodes/edges always come from the real loaded graph
    if (nodes > 0) m_labelNodes->setText(QString("Nodes: %1").arg(nodes));
    if (edges > 0) m_labelEdges->setText(QString("Edges: %1").arg(edges));

    const QString algo = m_algoSelection->currentText();
    if (triangles < 0)
        m_labelTriangles->setText("Triangles: N/A");
    else if (triangles == 0)
        m_labelTriangles->setText("Triangles: —");
    else if (algo == "Importance Sampling (Triangle Estimation)" && !m_pipelineNeedTriangles)
        m_labelTriangles->setText(QString("Triangles (est.): %1").arg(triangles));
    else
        m_labelTriangles->setText(QString("Triangles (exact): %1").arg(triangles));

    // Log graph load info in purple (triangles=0 means just loaded, no counting yet)
    if (triangles == 0 && nodes > 0) {
        logGraph(QString("Graph loaded: %1 | n=%2 m=%3")
            .arg(name).arg(nodes).arg(edges));
    }

    // Pipeline step 2 done → T_ref is now the real exact value, run sampling
    if (m_pipelineNeedTriangles && triangles > 0) {
        m_pipelineNeedTriangles = false;
        m_lastExactTriangles = triangles;       // store for gap report
        m_pendingSnapTriangles = triangles;     // use real computed value
        updateStatusLabel();
        QString path = m_pipelineFilePath;
        m_pipelineFilePath.clear();
        logInfo("Step 3/3: Running importance sampling with real T_ref...");
        m_runner->runImportanceSamplingEstimation(
            path, m_pendingSnapTriangles, m_pendingSnapArboricity);
        return;
    }

    if (triangles > 0 && algo != "Importance Sampling (Triangle Estimation)")
        m_pendingSnapTriangles = triangles;
    updateStatusLabel();

    // Final result logging and report
    if (triangles > 0) {
        double elapsed = QDateTime::currentMSecsSinceEpoch() - m_runStartMs;

        if (algo == "Importance Sampling (Triangle Estimation)" && !m_pipelineNeedTriangles) {
            m_lastEstimatedTriangles = triangles;
            int64_t exact = m_lastExactTriangles > 0 ? m_lastExactTriangles : m_pendingSnapTriangles;
            double gap = triangles - exact;
            double pct = (exact > 0) ? (gap / exact) * 100.0 : 0.0;
            QString pctStr = (pct >= 0 ? "+" : "") + QString::number(pct, 'f', 1) + "%";
            QString direction = (triangles > exact) ? "▲ over" : "▼ under";
            // Log order: exact first, then estimated (as discussed)
            logResult(QString("RESULT  exact     : %1").arg(exact));
            logResult(QString("        estimated : %1").arg(triangles));
            logResult(QString("        gap       : %1 (%2 by %3, %4)")
                .arg(gap, 0, 'f', 0)
                .arg(direction)
                .arg(std::abs(gap), 0, 'f', 0)
                .arg(pctStr));
            appendReport(name, algo, nodes, edges,
                         exact, triangles,
                         m_pendingSnapArboricity, elapsed);

        } else if (algo == "Exact Triangle Counting") {
            m_lastExactTriangles = triangles;
            logResult(QString("Graph: %1 | n=%2 m=%3 triangles=%4")
                .arg(name).arg(nodes).arg(edges).arg(triangles));
            appendReport(name, algo, nodes, edges,
                         triangles, 0,
                         m_pendingSnapArboricity, elapsed);

        } else if (algo == "Exact Arboricity") {
            logResult(QString("Graph: %1 | n=%2 m=%3 arboricity=%4")
                .arg(name).arg(nodes).arg(edges).arg(m_pendingSnapArboricity));
            appendReport(name, algo, nodes, edges,
                         0, 0,
                         m_pendingSnapArboricity, elapsed);
        }
    }
}