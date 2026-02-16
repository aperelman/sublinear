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

SnapBrowserWidget::SnapBrowserWidget(QWidget* p) 
    : QWidget(p)
    , dlmgr(new DownloadManager(this))
    , networkManager(new QNetworkAccessManager(this)) 
{
    setupUI();
    
    // Per-reply connections instead of blanket finished signal,
    // so detail page replies don't route to onScrapeFinished.
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
        ds.numTriangles = 0;  // Fetched from detail pages

        QString id = match.captured(1).section('/', -1).section('.', 0, 0);
        ds.url = QString("https://snap.stanford.edu/data/%1.txt.gz").arg(id);
        ds.detailsUrl = QString("https://snap.stanford.edu/data/%1.html").arg(id);
        ds.filename = id + ".txt.gz";
        datasets.append(ds);
    }

    if (datasets.isEmpty()) {
        info->setText("<b style='color:red;'>Error:</b> No datasets found. Website format might have changed.");
    } else {
        updateList();
        fetchTriangleCounts();
    }
    reply->deleteLater();
}

void SnapBrowserWidget::fetchTriangleCounts() {
    for (int i = 0; i < datasets.size(); ++i) {
        QString detailsUrl = datasets[i].detailsUrl;
        if (detailsUrl.isEmpty()) {
            detailsUrl = datasets[i].url;
            detailsUrl.replace(".txt.gz", ".html");
        }

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
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError || idx < 0 || idx >= datasets.size())
        return;

    QString html = reply->readAll();

    // SNAP detail pages use HTML table: <td>Number of triangles</td><td>1612010</td>
    QRegularExpression re(
        "Number of triangles\\s*</td>\\s*<td>\\s*(\\d[\\d,]*)",
        QRegularExpression::CaseInsensitiveOption
    );
    QRegularExpressionMatch m = re.match(html);

    /*
    // DEBUG — remove after testing
    qDebug() << "Detail page for" << idx << datasets[idx].name
             << "html length:" << html.length()
             << "match:" << m.hasMatch()
             << (m.hasMatch() ? m.captured(1) : "NO MATCH");
*/
    if (m.hasMatch()) {
        datasets[idx].numTriangles = m.captured(1).remove(',').toLongLong();

        if (list->currentRow() == idx)
            onDatasetSelected();

        saveToCache();
    }
}

void SnapBrowserWidget::onDatasetSelected() {
    int row = list->currentRow();
    if (row < 0 || row >= datasets.size()) return;

    const GraphInfo& ds = datasets.at(row);
    QLocale english(QLocale::English);

    double density = 0;
    if (ds.numNodes > 1) {
        density = (2.0 * ds.numEdges) / (double(ds.numNodes) * (double(ds.numNodes) - 1.0));
    }

    QString html = QString(
        "<html><body style='font-family: sans-serif; line-height: 2.0;'>"
        "  <h2 style='color: #2c3e50; margin-bottom: 5px;'>%1</h2>"
        "  <hr style='border: 0; border-top: 1px solid #eee;'>"
        "  <table width='100%%' cellpadding='15' cellspacing='0' style='margin-top: 10px;'>"
        "    <tr style='background-color: #fcfcfc;'>"
        "      <td style='color: #7f8c8d;'><b>Nodes:</b></td>"
        "      <td align='right' style='font-size: 11pt;'>%2</td>"
        "    </tr>"
        "    <tr>"
        "      <td style='color: #7f8c8d;'><b>Edges:</b></td>"
        "      <td align='right' style='font-size: 11pt;'>%3</td>"
        "    </tr>"
        "    <tr style='background-color: #fcfcfc;'>"
        "      <td style='color: #7f8c8d;'><b>Number of triangles:</b></td>"
        "      <td align='right' style='color: #e67e22; font-weight: bold; font-size: 12pt;'>%4</td>"
        "    </tr>"
        "    <tr>"
        "      <td style='color: #7f8c8d;'><b>Density:</b></td>"
        "      <td align='right' style='color: #95a5a6;'>%5</td>"
        "    </tr>"
        "  </table>"
        "  <div style='margin-top: 25px; padding: 12px; background: #f8f9fa; border-radius: 4px; color: #546e7a; font-size: 9pt;'>"
        "    <b>Local filename:</b> %6"
        "  </div>"
        "</body></html>"
    )
    .arg(ds.name)
    .arg(english.toString((qlonglong)ds.numNodes))
    .arg(english.toString((qlonglong)ds.numEdges))
    .arg(ds.numTriangles > 0 ? english.toString((qlonglong)ds.numTriangles) : "<i>Not available</i>")
    .arg(QString::number(density, 'g', 4))
    .arg(ds.filename);

    info->setText(html);
    btn->setEnabled(true);
    btn->setText(isDownloaded(ds) ? "Analyze Local Copy" : "Download Dataset");
}

void SnapBrowserWidget::updateList() {
    list->clear();
    for (const auto& ds : datasets) {
        list->addItem(ds.name);
    }
}

void SnapBrowserWidget::loadFromCache() {
    QFile file("snap_catalog.json");
    if (!file.open(QIODevice::ReadOnly)) return;
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonArray array = doc.array();
    
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
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(array).toJson());
    }
}

void SnapBrowserWidget::onRefreshClicked() {
    refreshBtn->setEnabled(false);
    QNetworkReply* reply = networkManager->get(
        QNetworkRequest(QUrl("https://snap.stanford.edu/data/index.html")));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onScrapeFinished(reply);
    });
}

void SnapBrowserWidget::onDownloadClicked() {
    int row = list->currentRow();
    if (row < 0) return;

    const auto& ds = datasets[row];
    QString path = QDir::currentPath() + "/data/" + ds.filename;

    if (QFile::exists(path)) {
        emit datasetReady(path);
        return;
    }

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