#include "snap_browser_widget.h"
#include "download_manager.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QFile>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QDir>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

static QString getIndexFilePath() {
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/data";
    QDir().mkpath(dataDir);
    return dataDir + "/snap_index.html";
}

SnapBrowserWidget::SnapBrowserWidget(DownloadManager *mgr, QWidget *parent)
    : QWidget(parent), downloadManager(mgr)
{
    datasetModel = new QStandardItemModel(this);
    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(datasetModel);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterKeyColumn(0);

    searchEdit = new QLineEdit(this);
    searchEdit->setPlaceholderText(tr("Search datasets..."));
    searchEdit->setClearButtonEnabled(true);
    searchEdit->setMinimumHeight(32);

    datasetView = new QTreeView(this);
    datasetView->setModel(proxyModel);
    datasetView->setRootIsDecorated(false);
    datasetView->header()->setVisible(false);
    datasetView->setSelectionBehavior(QAbstractItemView::SelectRows);
    datasetView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    datasetView->setAlternatingRowColors(true);
    datasetView->header()->setSectionResizeMode(QHeaderView::Stretch);

    refreshButton = new QPushButton(tr("Refresh SNAP Index"), this);

    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(6);
    layout->addWidget(searchEdit);
    layout->addWidget(datasetView, 1);
    layout->addWidget(refreshButton);

    connect(refreshButton, &QPushButton::clicked, this, &SnapBrowserWidget::onRefreshClicked);
    connect(searchEdit, &QLineEdit::textChanged, this, &SnapBrowserWidget::onSearchTextChanged);
    connect(datasetView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &SnapBrowserWidget::handleSelectionChanged);
    connect(datasetView, &QTreeView::doubleClicked, this, &SnapBrowserWidget::onDoubleClicked);

    if (downloadManager) {
        connect(downloadManager, &DownloadManager::catalogDownloaded, this, &SnapBrowserWidget::handleCatalogReady);
        connect(downloadManager, &DownloadManager::urlValidated, this, &SnapBrowserWidget::handleUrlValidated);
    }

    loadCache();
}

void SnapBrowserWidget::onSearchTextChanged(const QString &text) { proxyModel->setFilterFixedString(text); }

void SnapBrowserWidget::onRefreshClicked() {
    refreshButton->setEnabled(false);
    Q_EMIT logMessage("Fetching latest SNAP index...");
    downloadManager->startCatalogDownload(QUrl("https://snap.stanford.edu/data/index.html"), getIndexFilePath());
}

void SnapBrowserWidget::handleCatalogReady(bool success) {
    refreshButton->setEnabled(true);
    if (success) {
        QFile file(getIndexFilePath());
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            parseSnapHtml(file.readAll());
            file.close();
        }
    }
}

void SnapBrowserWidget::parseSnapHtml(const QString &html) {
    datasetModel->removeRows(0, datasetModel->rowCount());

    // Match table rows containing a dataset link plus two data columns (nodes, edges).
    // The original code used a corrupted '‹' character instead of '</' which caused
    // zero matches. Fixed to use proper </td> closing tags.
    QRegularExpression re(
        "<tr[^>]*>\\s*<td[^>]*>\\s*<a href=\"([^\"]*)\">([^<]+)</a>.*?<td[^>]*>([^<]*)</td>.*?<td[^>]*>([^<]*)</td>",
        QRegularExpression::DotMatchesEverythingOption
    );

    QRegularExpressionMatchIterator i = re.globalMatch(html);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString name = match.captured(2).trimmed();
        if (!name.isEmpty()) {
            auto *nameItem = new QStandardItem(name);
            nameItem->setData(match.captured(3).trimmed().replace(",", ""), Qt::UserRole + 1);
            nameItem->setData(match.captured(4).trimmed().replace(",", ""), Qt::UserRole + 2);
            nameItem->setData(qlonglong(m_trianglesCache.value(name, -1)), Qt::UserRole + 3);

            QUrl downloadUrl("https://snap.stanford.edu/data/" + name + ".txt.gz");
            nameItem->setData(downloadUrl, Qt::UserRole);
            datasetModel->appendRow(nameItem);

            if (downloadManager) downloadManager->validateUrl(downloadUrl);
        }
    }
}

void SnapBrowserWidget::handleUrlValidated(const QUrl &url, bool isValid) {
    if (isValid) return;

    for (int i = 0; i < datasetModel->rowCount(); ++i) {
        if (datasetModel->item(i)->data(Qt::UserRole).toUrl() == url) {
            datasetModel->removeRow(i);
            break;
        }
    }
}

void SnapBrowserWidget::handleSelectionChanged(const QItemSelection &selected, const QItemSelection &) {
    if (selected.indexes().isEmpty()) return;
    QModelIndex sourceIndex = proxyModel->mapToSource(selected.indexes().first());
    if (!sourceIndex.isValid()) return;

    QStandardItem* item = datasetModel->itemFromIndex(sourceIndex);
    QString name = item->text();
    QUrl downloadUrl = item->data(Qt::UserRole).toUrl();
    int64_t triangles = item->data(Qt::UserRole + 3).toLongLong();

    // If we already have triangles (cached), emit datasetSelected immediately
    if (triangles != -1) {
        Q_EMIT datasetSelected(name, downloadUrl, triangles);
        return;
    }

    // Otherwise, fetch the deep page to get triangle count
    QUrl subPageUrl("https://snap.stanford.edu/data/" + name + ".html");
    QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/" + name + ".html";

    connect(downloadManager, &DownloadManager::finished, this, [this, item, name, downloadUrl, tempPath](const QString &path) {
        if (path == tempPath) {
            QFile file(path);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                parseDeepPage(file.readAll(), item);
                file.close();
                QFile::remove(path);
            }
            int64_t newTriangles = item->data(Qt::UserRole + 3).toLongLong();
            Q_EMIT datasetSelected(name, downloadUrl, newTriangles);
        }
    }, Qt::SingleShotConnection);

    downloadManager->startDownload(subPageUrl, tempPath);
}

void SnapBrowserWidget::parseDeepPage(const QString &html, QStandardItem *item) {
    // Fixed: was using corrupted '‹' instead of '</td>' as closing tag delimiter.
    QRegularExpression triRegex(
        "Triangles.*?<td[^>]*>([\\d,]+)</td>",
        QRegularExpression::DotMatchesEverythingOption | QRegularExpression::CaseInsensitiveOption
    );
    QRegularExpressionMatch match = triRegex.match(html);
    int64_t triangles = match.hasMatch() ? match.captured(1).replace(",", "").toLongLong() : 0;
    item->setData(qlonglong(triangles), Qt::UserRole + 3);
    m_trianglesCache[item->text()] = triangles;
    saveCache();
    Q_EMIT datasetMetadataLoaded(item->text(),
                                 item->data(Qt::UserRole + 1).toLongLong(),
                                 item->data(Qt::UserRole + 2).toLongLong(),
                                 triangles);
}

void SnapBrowserWidget::onDoubleClicked(const QModelIndex &proxyIndex) {
    QModelIndex sourceIndex = proxyModel->mapToSource(proxyIndex);
    if (sourceIndex.isValid()) {
        QStandardItem* item = datasetModel->itemFromIndex(sourceIndex);
        Q_EMIT downloadRequested(item->text(), item->data(Qt::UserRole).toUrl());
    }
}

QUrl SnapBrowserWidget::getUrlForDataset(const QString& filename) const {
    auto items = datasetModel->findItems(filename, Qt::MatchExactly | Qt::MatchRecursive, 0);
    return items.isEmpty() ? QUrl() : items.first()->data(Qt::UserRole).toUrl();
}

void SnapBrowserWidget::loadCache() {
    QFile file(cacheFilePath());
    if (!file.open(QIODevice::ReadOnly)) return;
    QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    for (const QJsonValue &val : root["triangles"].toArray()) {
        m_trianglesCache[val.toObject()["name"].toString()] = val.toObject()["triangles"].toVariant().toLongLong();
    }
}

void SnapBrowserWidget::saveCache() {
    QJsonArray arr;
    for (auto it = m_trianglesCache.constBegin(); it != m_trianglesCache.constEnd(); ++it) {
        QJsonObject obj; obj["name"] = it.key(); obj["triangles"] = qint64(it.value());
        arr.append(obj);
    }
    QJsonObject root; root["triangles"] = arr;
    QFile file(cacheFilePath());
    QDir().mkpath(QFileInfo(file).absolutePath());
    if (file.open(QIODevice::WriteOnly)) file.write(QJsonDocument(root).toJson());
}

QString SnapBrowserWidget::cacheFilePath() const {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/cache/snap_cache.json";
}
