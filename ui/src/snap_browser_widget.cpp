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
#include <QStandardPaths>

SnapBrowserWidget::SnapBrowserWidget(QWidget* p)
    : QWidget(p)
    , dlmgr(new DownloadManager(this))
    , networkManager(new QNetworkAccessManager(this))
{
    setupUI();

    connect(dlmgr, &DownloadManager::downloadProgress, this, &SnapBrowserWidget::onDownloadProgress);
    connect(dlmgr, &DownloadManager::downloadFinished, this, &SnapBrowserWidget::onDownloadFinished);
    connect(dlmgr, &DownloadManager::downloadError,    this, &SnapBrowserWidget::onDownloadError);

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

    connect(list,       &QListWidget::itemSelectionChanged, this, &SnapBrowserWidget::onDatasetSelected);
    connect(btn,        &QPushButton::clicked,              this, &SnapBrowserWidget::onDownloadClicked);
    connect(refreshBtn, &QPushButton::clicked,              this, &SnapBrowserWidget::onRefreshClicked);
}

void SnapBrowserWidget::handleAnalysis(const QString& filePath) {
    if (filePath.isEmpty()) return;
    QMessageBox::information(this, "Analysis", "Starting analysis on: " + filePath);
}


static QString dataPath() {
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/data";
    QDir().mkpath(path);
    return path;
}

bool SnapBrowserWidget::hasSelection() const {
    return list->currentRow() >= 0;
}

QString SnapBrowserWidget::selectedFilePath() const {
    int row = list->currentRow();
    if (row < 0 || row >= datasets.size()) return {};
    return datasets.at(row).localPath;
}

// ---------------------------------------------------------------------------
// Dataset selection
// ---------------------------------------------------------------------------

void SnapBrowserWidget::onDatasetSelected() {
    int row = list->currentRow();
    if (row < 0 || row >= datasets.size()) return;

    GraphInfo& ds = datasets[row];
    QLocale english(QLocale::English);

    double density = (ds.numNodes > 1)
        ? (2.0 * ds.numEdges) / (double(ds.numNodes) * (double(ds.numNodes) - 1.0))
        : 0.0;

    QString triangleStr = ds.numTriangles > 0
        ? english.toString((qlonglong)ds.numTriangles)
        : "N/A";

    // Truncate description to 2 lines
    QString desc = ds.description.isEmpty() ? "No description available" : ds.description;
    if (desc.length() > 120) desc = desc.left(117) + "...";

    QString text = QString(
        "<b style='font-size:13pt;'>%1</b><br>"
        "<i style='color:#222; font-size:10pt;'>%2</i><br><br>"
        "<span style='font-size:11pt; line-height:200%;'>Nodes: %3<br>"
        "Edges: %4<br>"
        "<span style='color:#1a7abf; font-size:11pt;'><b>Triangles: %5</b></span><br>"
        "Density: %6</span><br><br>"
        "<small style='color:#888;'>%7</small>")
        .arg(ds.name)
        .arg(desc)
        .arg(english.toString((qlonglong)ds.numNodes))
        .arg(english.toString((qlonglong)ds.numEdges))
        .arg(triangleStr)
        .arg(QString::number(density, 'g', 4))
        .arg(ds.filename);

    info->setText(text);
    btn->setEnabled(true);

    QString path = dataPath() + "/" + ds.filename;
    if (QFile::exists(path)) {
        ds.localPath = path;
        btn->setText("Activate Local Copy");
        emit datasetReady(path);
    } else {
        btn->setText("Download Dataset");
    }

    // Always notify MainWindow that a selection was made
    emit datasetSelected();
}

// ---------------------------------------------------------------------------
// Download / activate
// ---------------------------------------------------------------------------

void SnapBrowserWidget::onDownloadClicked() {
    int row = list->currentRow();
    if (row < 0) return;

    GraphInfo& ds = datasets[row];
    QString path = dataPath() + "/" + ds.filename;

    // File already on disk — activate it immediately
    if (QFile::exists(path)) {
        ds.localPath = path;
        saveToCache();
        emit datasetReady(path);
        btn->setText("Activate Local Copy");
        return;
    }

    // Start download
    btn->setEnabled(false);
    progress->setVisible(true);
    QDir().mkpath(dataPath());
    dlmgr->downloadFile(ds.url, path);
}

void SnapBrowserWidget::onDownloadProgress(qint64 received, qint64 total) {
    if (total > 0)
        progress->setValue((received * 100) / total);
}

void SnapBrowserWidget::onDownloadFinished(const QString& filePath) {
    progress->setVisible(false);
    btn->setEnabled(true);

    // Persist localPath in matching dataset entry
    for (GraphInfo& ds : datasets) {
        if (dataPath() + "/" + ds.filename == filePath) {
            ds.localPath = filePath;
            break;
        }
    }
    saveToCache();

    emit datasetReady(filePath);
    btn->setText("Activate Local Copy");
}

void SnapBrowserWidget::onDownloadError(const QString& errorMsg) {
    progress->setVisible(false);
    btn->setEnabled(true);
    QMessageBox::warning(this, "Download Error", errorMsg);
}

// ---------------------------------------------------------------------------
// Refresh / scrape from SNAP website
// ---------------------------------------------------------------------------

void SnapBrowserWidget::onRefreshClicked() {
    refreshBtn->setEnabled(false);
    QNetworkReply* reply = networkManager->get(
        QNetworkRequest(QUrl("https://snap.stanford.edu/data/index.html")));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onScrapeFinished(reply);
    });
}

void SnapBrowserWidget::onScrapeFinished(QNetworkReply* reply) {
    refreshBtn->setEnabled(true);
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::critical(this, "Network Error", reply->errorString());
        reply->deleteLater();
        return;
    }

    QString html = reply->readAll();
    QRegularExpression re(
        "<tr.*?>\\s*<td><a href=\"(.*?)\">(.*?)</a></td>"
        "\\s*<td>.*?</td>"
        "\\s*<td>(.*?)</td>"
        "\\s*<td>(.*?)</td>"
        "\\s*<td>(.*?)</td>");

    QRegularExpressionMatchIterator it = re.globalMatch(html);
    datasets.clear();

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        GraphInfo ds;
        ds.name         = match.captured(2).trimmed();
        ds.numNodes     = match.captured(3).remove(',').trimmed().toLongLong();
        ds.numEdges     = match.captured(4).remove(',').trimmed().toLongLong();
        ds.numTriangles = 0;

        QString id    = match.captured(1).section('/', -1).section('.', 0, 0);
        ds.url        = QString("https://snap.stanford.edu/data/%1.txt.gz").arg(id);
        ds.detailsUrl = QString("https://snap.stanford.edu/data/%1.html").arg(id);
        ds.filename   = id + ".txt.gz";

        // Restore localPath if file still exists on disk
        QString path = dataPath() + "/" + ds.filename;
        if (QFile::exists(path))
            ds.localPath = path;

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
        QNetworkRequest req{QUrl(datasets[i].detailsUrl)};
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
        bool changed = false;

        // Extract triangle count
        QRegularExpression reTri(
            "Number of triangles\\s*</td>\\s*<td>\\s*(\\d[\\d,]*)",
            QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch m = reTri.match(html);
        if (m.hasMatch()) {
            datasets[idx].numTriangles = m.captured(1).remove(',').toLongLong();
            changed = true;
        }

        // Extract description - the first paragraph after "Dataset information"
        QRegularExpression reDesc(
            "Dataset information.*?<p[^>]*>(.*?)</p>",
            QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
        QRegularExpressionMatch md = reDesc.match(html);
        if (md.hasMatch()) {
            // Strip any inner HTML tags and trim
            QString desc = md.captured(1);
            desc.remove(QRegularExpression("<[^>]+>"));
            desc = desc.simplified();
            if (!desc.isEmpty()) {
                datasets[idx].description = desc;
                changed = true;
            }
        }

        // Extract the best download URL from the Files table:
        // Prefer *_combined.txt.gz, fall back to any .txt.gz, then .tar.gz
        QRegularExpression reFile(
            "<a href=\"([^\"]+\\.(?:txt\\.gz|tar\\.gz))\"",
            QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatchIterator it = reFile.globalMatch(html);

        QString bestUrl;
        while (it.hasNext()) {
            QRegularExpressionMatch fm = it.next();
            QString href = fm.captured(1);
            // Make absolute URL if relative
            if (!href.startsWith("http"))
                href = "https://snap.stanford.edu/data/" + href;
            // Prefer combined txt.gz
            if (href.contains("_combined.txt.gz", Qt::CaseInsensitive)) {
                bestUrl = href;
                break;
            }
            // Accept any txt.gz or tar.gz as fallback
            if (bestUrl.isEmpty())
                bestUrl = href;
        }

        if (!bestUrl.isEmpty()) {
            datasets[idx].url      = bestUrl;
            datasets[idx].filename = bestUrl.section('/', -1);
            changed = true;
        }

        if (changed) {
            if (list->currentRow() == idx)
                onDatasetSelected();
            saveToCache();
        }
    }
    reply->deleteLater();
}

// ---------------------------------------------------------------------------
// List / cache helpers
// ---------------------------------------------------------------------------

void SnapBrowserWidget::updateList() {
    list->clear();
    for (const auto& ds : datasets)
        list->addItem(ds.name);
}

void SnapBrowserWidget::loadFromCache() {
    QFile file("snap_catalog.json");
    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonArray array = QJsonDocument::fromJson(file.readAll()).array();
    datasets.clear();

    for (auto val : array) {
        QJsonObject obj = val.toObject();
        GraphInfo ds;
        ds.name         = obj["name"].toString();
        ds.numNodes     = obj["nodes"].toVariant().toLongLong();
        ds.numEdges     = obj["edges"].toVariant().toLongLong();
        ds.numTriangles = obj["triangles"].toVariant().toLongLong();
        ds.url          = obj["url"].toString();
        ds.detailsUrl   = obj["detailsUrl"].toString();
        ds.filename     = obj["filename"].toString();
        ds.localPath    = obj["localPath"].toString();
        ds.description  = obj["description"].toString();

        // Validate that the local file still exists
        if (!ds.localPath.isEmpty() && !QFile::exists(ds.localPath))
            ds.localPath.clear();

        datasets.append(ds);
    }
    updateList();
}

void SnapBrowserWidget::saveToCache() {
    QJsonArray array;
    for (const auto& ds : datasets) {
        QJsonObject obj;
        obj["name"]       = ds.name;
        obj["nodes"]      = (qint64)ds.numNodes;
        obj["edges"]      = (qint64)ds.numEdges;
        obj["triangles"]  = (qint64)ds.numTriangles;
        obj["url"]        = ds.url;
        obj["detailsUrl"] = ds.detailsUrl;
        obj["filename"]   = ds.filename;
        obj["localPath"]  = ds.localPath;
        obj["description"] = ds.description;
        array.append(obj);
    }
    QFile file("snap_catalog.json");
    if (file.open(QIODevice::WriteOnly))
        file.write(QJsonDocument(array).toJson());
}

bool SnapBrowserWidget::isDownloaded(const GraphInfo& ds) {
    return !ds.localPath.isEmpty() || QFile::exists(dataPath() + "/" + ds.filename);
}
