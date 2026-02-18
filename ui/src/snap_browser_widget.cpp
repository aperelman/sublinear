#include "snap_browser_widget.h"
#include "graph_info.h"
#include "download_manager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QLocale>
#include <QLabel>
#include <QProgressBar>

SnapBrowserWidget::SnapBrowserWidget(QWidget* p) 
    : QWidget(p)
    , dlmgr(new DownloadManager(this))
    , networkManager(new QNetworkAccessManager(this)) 
{
    setupUI();
    
    connect(dlmgr, &DownloadManager::downloadProgress, this, &SnapBrowserWidget::onDownloadProgress);
    connect(dlmgr, &DownloadManager::downloadFinished, this, &SnapBrowserWidget::onDownloadFinished);
    connect(dlmgr, &DownloadManager::downloadError, this, &SnapBrowserWidget::onDownloadError);

    loadFromCache();
}

void SnapBrowserWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);
    
    auto* headerLayout = new QHBoxLayout();
    headerLayout->addWidget(new QLabel("<b>SNAP Dataset Browser</b>"));
    
    refreshBtn = new QPushButton("Refresh from Web");
    headerLayout->addWidget(refreshBtn);
    layout->addLayout(headerLayout);

    list = new QListWidget();
    layout->addWidget(list);

    info = new QLabel("Select a dataset to view detailed information");
    info->setWordWrap(true);
    info->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    info->setStyleSheet("background-color: white; border: 1px solid #ddd; padding: 15px;");
    layout->addWidget(info);

    btn = new QPushButton("Download");
    btn->setEnabled(false);
    layout->addWidget(btn);

    progress = new QProgressBar();
    progress->setVisible(false);
    layout->addWidget(progress);

    connect(list, &QListWidget::itemSelectionChanged, this, &SnapBrowserWidget::onDatasetSelected);
    connect(btn, &QPushButton::clicked, this, &SnapBrowserWidget::onDownloadClicked);
    connect(refreshBtn, &QPushButton::clicked, this, &SnapBrowserWidget::onRefreshClicked);
}

void SnapBrowserWidget::handleAnalysis(const QString& filePath) {
    // Basic implementation to satisfy header and avoid future linker errors
    if (filePath.isEmpty()) return;
    QMessageBox::information(this, "Analysis", "Starting analysis on: " + filePath);
}

void SnapBrowserWidget::onScrapeFinished(QNetworkReply* reply) {
    refreshBtn->setEnabled(true);
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::critical(this, "Network Error", reply->errorString());
        reply->deleteLater();
        return;
    }

    QString html = reply->readAll();
    QRegularExpression re("<tr.*?>\\s*<td><a href=\"(.*?)\">(.*?)</a></td>\\s*<td>.*?</td>\\s*<td>(.*?)</td>\\s*<td>(.*?)</td>\\s*<td>(.*?)</td>");
    QRegularExpressionMatchIterator i = re.globalMatch(html);

    datasets.clear();
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        GraphInfo ds;
        ds.name = match.captured(2).trimmed();
        ds.numNodes = match.captured(3).remove(',').trimmed().toLongLong();
        ds.numEdges = match.captured(4).remove(',').trimmed().toLongLong();
        ds.numTriangles = 0; 

        QString id = match.captured(1).section('/', -1).section('.', 0, 0);
        ds.url = QString("https://snap.stanford.edu/data/%1.txt.gz").arg(id);
        ds.detailsUrl = QString("https://snap.stanford.edu/data/%1.html").arg(id);
        ds.filename = id + ".txt.gz";
        datasets.append(ds);
    }

    if (datasets.isEmpty()) {
        info->setText("<b style='color:red;'>Error:</b> No datasets found.");
    } else {
        updateList();
        fetchTriangleCounts();
    }
    reply->deleteLater();
}

void SnapBrowserWidget::fetchTriangleCounts() {
    for (int i = 0; i < datasets.size(); ++i) {
        QString detailsUrl = datasets[i].detailsUrl;
        QNetworkRequest req{QUrl(detailsUrl)};
        QNetworkReply* reply = networkManager->get(req);
        reply->setProperty("datasetIndex", i);

        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            onDetailPageFinished(reply);
        });
    }
}

void SnapBrowserWidget::onDetailPageFinished(QNetworkReply* reply) {
    int idx = reply->property("datasetIndex").toInt();
    if (reply->error() == QNetworkReply::NoError && idx >= 0 && idx < datasets.size()) {
        QString html = reply->readAll();
        QRegularExpression re("Number of triangles\\s*</td>\\s*<td>\\s*(\\d[\\d,]*)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch m = re.match(html);

        if (m.hasMatch()) {
            datasets[idx].numTriangles = m.captured(1).remove(',').toLongLong();
            if (list->currentRow() == idx) onDatasetSelected();
            saveToCache();
        }
    }
    reply->deleteLater();
}

void SnapBrowserWidget::onDatasetSelected() {
    int row = list->currentRow();
    if (row < 0 || row >= datasets.size()) return;

    const GraphInfo& ds = datasets.at(row);
    QLocale english(QLocale::English);

    double density = (ds.numNodes > 1) ? (2.0 * ds.numEdges) / (double(ds.numNodes) * (double(ds.numNodes) - 1.0)) : 0;

    QString html = QString("<html><body style='font-family: sans-serif;'>"
                           "<h2>%1</h2><hr>"
                           "<b>Nodes:</b> %2<br><b>Edges:</b> %3<br>"
                           "<b>Triangles:</b> %4<br><b>Density:</b> %5<br>"
                           "<small>File: %6</small></body></html>")
                   .arg(ds.name).arg(english.toString((qlonglong)ds.numNodes))
                   .arg(english.toString((qlonglong)ds.numEdges))
                   .arg(ds.numTriangles > 0 ? english.toString((qlonglong)ds.numTriangles) : "N/A")
                   .arg(QString::number(density, 'g', 4)).arg(ds.filename);

    info->setText(html);
    btn->setEnabled(true);
    btn->setText(isDownloaded(ds) ? "Analyze Local Copy" : "Download Dataset");
}

void SnapBrowserWidget::updateList() {
    list->clear();
    for (const auto& ds : datasets) list->addItem(ds.name);
}

void SnapBrowserWidget::loadFromCache() {
    QFile file("snap_catalog.json");
    if (!file.open(QIODevice::ReadOnly)) return;
    QJsonArray array = QJsonDocument::fromJson(file.readAll()).array();
    datasets.clear();
    for (auto val : array) {
        QJsonObject obj = val.toObject();
        GraphInfo ds;
        ds.name = obj["name"].toString();
        ds.numNodes = obj["nodes"].toVariant().toLongLong();
        ds.numEdges = obj["edges"].toVariant().toLongLong();
        ds.numTriangles = obj["triangles"].toVariant().toLongLong();
        ds.url = obj["url"].toString();
        ds.detailsUrl = obj["detailsUrl"].toString();
        ds.filename = obj["filename"].toString();
        datasets.append(ds);
    }
    updateList();
}

void SnapBrowserWidget::saveToCache() {
    QJsonArray array;
    for (const auto& ds : datasets) {
        QJsonObject obj;
        obj["name"] = ds.name;
        obj["nodes"] = (qint64)ds.numNodes;
        obj["edges"] = (qint64)ds.numEdges;
        obj["triangles"] = (qint64)ds.numTriangles;
        obj["url"] = ds.url;
        obj["detailsUrl"] = ds.detailsUrl;
        obj["filename"] = ds.filename;
        array.append(obj);
    }
    QFile file("snap_catalog.json");
    if (file.open(QIODevice::WriteOnly)) file.write(QJsonDocument(array).toJson());
}

void SnapBrowserWidget::onRefreshClicked() {
    refreshBtn->setEnabled(false);
    QNetworkReply* reply = networkManager->get(QNetworkRequest(QUrl("https://snap.stanford.edu/data/index.html")));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { onScrapeFinished(reply); });
}

void SnapBrowserWidget::onDownloadClicked() {
    int row = list->currentRow();
    if (row < 0) return;
    const auto& ds = datasets[row];
    QString path = QDir::currentPath() + "/data/" + ds.filename;
    if (QFile::exists(path)) { emit datasetReady(path); return; }
    btn->setEnabled(false);
    progress->setVisible(true);
    dlmgr->downloadFile(ds.url, path);
}

void SnapBrowserWidget::onDownloadProgress(qint64 r, qint64 t) {
    if (t > 0) progress->setValue((r * 100) / t);
}

void SnapBrowserWidget::onDownloadFinished(const QString& f) {
    progress->setVisible(false);
    btn->setEnabled(true);
    emit datasetReady(f);
}

void SnapBrowserWidget::onDownloadError(const QString& e) {
    progress->setVisible(false);
    btn->setEnabled(true);
    QMessageBox::warning(this, "Download Error", e);
}

bool SnapBrowserWidget::isDownloaded(const GraphInfo& ds) {
    return QFile::exists(QDir::currentPath() + "/data/" + ds.filename);
}