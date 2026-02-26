#include "mainwindow.h"
#include "graph_list_widget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QTime>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QRegularExpression>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    m_networkManager = new QNetworkAccessManager(this);
    setupManualLayout();
    loadInitialData();
    setWindowTitle("GraphAnalyzer Pro 2026");
    resize(1200, 800);
}

void MainWindow::setupManualLayout() {
    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    QVBoxLayout *root = new QVBoxLayout(central);
    QSplitter *splitter = new QSplitter(Qt::Horizontal);

    // Left Section
    QWidget *left = new QWidget();
    QVBoxLayout *lLayout = new QVBoxLayout(left);
    m_tabWidget = new QTabWidget();
    m_localList = new QListWidget();
    m_tabWidget->addTab(m_localList, "Local Files");

    QWidget *snapPg = new QWidget();
    QVBoxLayout *sLay = new QVBoxLayout(snapPg);
    m_btnRefreshSnap = new QPushButton("Sync SNAP Names");
    m_graphList = new GraphListWidget();
    sLay->addWidget(m_btnRefreshSnap);
    sLay->addWidget(m_graphList);
    m_tabWidget->addTab(snapPg, "SNAP Web");
    lLayout->addWidget(m_tabWidget);

    QGroupBox *box = new QGroupBox("Dataset Properties");
    QVBoxLayout *bLay = new QVBoxLayout(box);
    m_lblNodes = new QLabel("Nodes: -");
    m_lblEdges = new QLabel("Edges: -");
    m_lblTriangles = new QLabel("Triangles: -");
    m_btnDownload = new QPushButton("Download Dataset");
    m_btnDownload->setEnabled(false);
    bLay->addWidget(m_lblNodes); bLay->addWidget(m_lblEdges); bLay->addWidget(m_lblTriangles);
    bLay->addWidget(m_btnDownload);
    lLayout->addWidget(box);

    // Right Section
    QWidget *right = new QWidget();
    QVBoxLayout *rLay = new QVBoxLayout(right);
    m_algoCombo = new QComboBox();
    m_algoCombo->addItems({"-- Select Algorithm --", "Exact Arboricity", "Triangle Counting"});
    m_runBtn = new QPushButton("Run Analysis");
    m_runBtn->setEnabled(false);
    rLay->addWidget(new QLabel("Algorithm Selection:"));
    rLay->addWidget(m_algoCombo);
    rLay->addStretch();
    rLay->addWidget(m_runBtn);

    splitter->addWidget(left);
    splitter->addWidget(right);
    root->addWidget(splitter, 1);

    m_logArea = new QTextEdit();
    m_logArea->setReadOnly(true);
    m_logArea->setStyleSheet("background: #121212; color: #00FF00; font-family: Consolas;");
    root->addWidget(new QLabel("System Log:"));
    root->addWidget(m_logArea);

    // Connections
    connect(m_btnRefreshSnap, &QPushButton::clicked, this, &MainWindow::onRefreshSnapRequested);
    connect(m_btnDownload, &QPushButton::clicked, this, &MainWindow::onDownloadClicked);
    connect(m_graphList, &GraphListWidget::requestMetadata, this, &MainWindow::onSnapGraphSelected);
    connect(m_localList, &QListWidget::currentTextChanged, this, &MainWindow::onLocalItemSelected);
    connect(m_algoCombo, &QComboBox::currentIndexChanged, [this](int i){ m_runBtn->setEnabled(i > 0); });
}

void MainWindow::onSnapGraphSelected(const QString& name, const QString& urlPath) {
    m_btnDownload->setEnabled(false);
    QString pathCopy = urlPath;
    m_currentSnapUrl = "https://snap.stanford.edu/data/" + pathCopy.replace(".html", ".txt.gz");

    appendLog("Fetching metadata for: " + name);
    QNetworkReply* reply = m_networkManager->get(QNetworkRequest(QUrl("https://snap.stanford.edu/data/" + urlPath)));

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            parseSubPageMetadata(reply->readAll());
            m_btnDownload->setEnabled(true);
        }
        reply->deleteLater();
    });
}

void MainWindow::parseSubPageMetadata(const QString& html) {
    auto extract = [&](QString key) {
        QRegularExpression re(key + "</td><td>(\\d+)");
        QRegularExpressionMatch m = re.match(html);
        return m.hasMatch() ? m.captured(1) : "N/A";
    };
    m_lblNodes->setText("Nodes: " + extract("Nodes"));
    m_lblEdges->setText("Edges: " + extract("Edges"));
    m_lblTriangles->setText("Triangles: " + extract("Triangles"));
}

void MainWindow::onDownloadClicked() {
    appendLog("Starting Download: " + m_currentSnapUrl);
    m_btnDownload->setText("Downloading...");
    m_btnDownload->setEnabled(false);

    QNetworkReply* reply = m_networkManager->get(QNetworkRequest(QUrl(m_currentSnapUrl)));
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QDir().mkdir("data");
            QString name = m_currentSnapUrl.split('/').last();
            QFile f("data/" + name);
            if (f.open(QIODevice::WriteOnly)) {
                f.write(reply->readAll());
                f.close();
                appendLog("Saved successfully: " + name);
                m_localList->addItem(name);
            }
        } else {
            appendLog("Download Error: " + reply->errorString());
        }
        m_btnDownload->setText("Download Dataset");
        m_btnDownload->setEnabled(true);
        reply->deleteLater();
    });
}

void MainWindow::onRefreshSnapRequested() { m_graphList->fetchNamesOnly(); }
void MainWindow::onLocalItemSelected(const QString &n) { appendLog("Context switched to: " + n); }
void MainWindow::onRunClicked() { appendLog("Execution started..."); }

void MainWindow::appendLog(const QString& m) {
    if (m_logArea) m_logArea->append(QString("[%1] %2").arg(QTime::currentTime().toString()).arg(m));
}

void MainWindow::loadInitialData() {
    if (QFile::exists("cache.json")) appendLog("Cache loaded.");
}

MainWindow::~MainWindow() {}