#include "snap_browser_widget.h"
#include "download_manager.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QDir>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QLocale>
#include <QDebug>

// Column indices
enum Col {
    ColName       = 0,
    ColNodes      = 1,
    ColEdges      = 2,
    ColTriangles  = 3,
    ColDownloaded = 4,
    ColCount      = 5
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
QString SnapBrowserWidget::formatNumber(int64_t n) {
    if (n <= 0) return "–";
    return QLocale().toString((qlonglong)n);
}

QString SnapBrowserWidget::getIndexFilePath() {
    QString dataDir = QStandardPaths::writableLocation(
                          QStandardPaths::AppDataLocation) + "/data";
    QDir().mkpath(dataDir);
    return dataDir + "/snap_index.html";
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
SnapBrowserWidget::SnapBrowserWidget(DownloadManager *mgr, QWidget *parent)
    : QWidget(parent), downloadManager(mgr)
{
    // Load bundled catalog first so rows have stats immediately
    loadBundledCatalog();

    datasetModel = new QStandardItemModel(this);
    datasetModel->setHorizontalHeaderLabels({
        tr("Dataset"), tr("Nodes"), tr("Edges"), tr("Triangles"), tr("Downloaded")
    });

    proxyModel = new NumericSortProxyModel(this);
    proxyModel->setSourceModel(datasetModel);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterKeyColumn(ColName);

    searchEdit = new QLineEdit(this);
    searchEdit->setPlaceholderText(tr("Search datasets..."));
    searchEdit->setClearButtonEnabled(true);
    searchEdit->setMinimumHeight(32);

    datasetView = new QTreeView(this);
    datasetView->setModel(proxyModel);
    datasetView->setRootIsDecorated(false);
    datasetView->header()->setVisible(true);
    datasetView->header()->setSortIndicatorShown(true);
    datasetView->header()->setSectionsClickable(true);
    datasetView->setSortingEnabled(true);
    datasetView->sortByColumn(ColName, Qt::AscendingOrder);
    datasetView->setSelectionBehavior(QAbstractItemView::SelectRows);
    datasetView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    datasetView->setAlternatingRowColors(true);

    datasetView->header()->setSectionResizeMode(ColName,       QHeaderView::Stretch);
    datasetView->header()->setSectionResizeMode(ColNodes,      QHeaderView::ResizeToContents);
    datasetView->header()->setSectionResizeMode(ColEdges,      QHeaderView::ResizeToContents);
    datasetView->header()->setSectionResizeMode(ColTriangles,  QHeaderView::ResizeToContents);
    datasetView->header()->setSectionResizeMode(ColDownloaded, QHeaderView::ResizeToContents);

    refreshButton = new QPushButton(tr("Refresh SNAP Index"), this);

    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(6);
    layout->addWidget(searchEdit);
    layout->addWidget(datasetView, 1);
    layout->addWidget(refreshButton);

    connect(refreshButton,  &QPushButton::clicked,
            this, &SnapBrowserWidget::onRefreshClicked);
    connect(searchEdit, &QLineEdit::textChanged,
            this, &SnapBrowserWidget::onSearchTextChanged);
    connect(datasetView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &SnapBrowserWidget::handleSelectionChanged);
    connect(datasetView, &QTreeView::doubleClicked,
            this, &SnapBrowserWidget::onDoubleClicked);
    connect(datasetView->header(), &QHeaderView::sortIndicatorChanged,
            this, &SnapBrowserWidget::onSortIndicatorChanged);

    if (downloadManager) {
        connect(downloadManager, &DownloadManager::catalogDownloaded,
                this, &SnapBrowserWidget::handleCatalogReady);
        connect(downloadManager, &DownloadManager::urlValidated,
                this, &SnapBrowserWidget::handleUrlValidated);
    }

    // Populate from bundled catalog immediately — no network needed
    for (const auto& d : m_bundledCatalog)
        addDatasetRow(d.id, d.name, d.nodes, d.edges, d.triangles);

    emit logMessage(QString("Loaded %1 datasets from bundled catalog")
                    .arg(datasetModel->rowCount()));

    loadCache(); // overlay any cached triangle counts
}

SnapBrowserWidget::~SnapBrowserWidget() {
    disconnect();
}

// ---------------------------------------------------------------------------
// Bundled catalog — loaded from Qt resource :/data/snap_datasets.json
// ---------------------------------------------------------------------------
void SnapBrowserWidget::loadBundledCatalog() {
    m_bundledCatalog.clear();

    QFile f(":/data/snap_datasets.json");
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "snap_datasets.json not found in Qt resources";
        return;
    }

    QJsonArray arr = QJsonDocument::fromJson(f.readAll()).array();
    for (const QJsonValue& v : arr) {
        QJsonObject o = v.toObject();
        BundledDataset d;
        d.id        = o["id"].toString();
        d.name      = o["name"].toString();
        d.nodes     = (int64_t)o["nodes"].toDouble();
        d.edges     = (int64_t)o["edges"].toDouble();
        d.triangles = (int64_t)o["triangles"].toDouble();
        d.type      = o["type"].toString();
        if (!d.id.isEmpty())
            m_bundledCatalog.append(d);
    }
}

const BundledDataset* SnapBrowserWidget::findBundled(const QString& id) const {
    for (const auto& d : m_bundledCatalog)
        if (d.id == id) return &d;
    return nullptr;
}

// ---------------------------------------------------------------------------
// Add a single dataset row to the model
// ---------------------------------------------------------------------------
void SnapBrowserWidget::addDatasetRow(const QString& id, const QString& name,
                                      int64_t nodes, int64_t edges, int64_t tris)
{
    QString downloadDir = QStandardPaths::writableLocation(
                              QStandardPaths::DownloadLocation);

    int64_t cachedTris = m_trianglesCache.value(name, -1);
    if (cachedTris >= 0) tris = cachedTris;

    // Use resolved URL if available, otherwise placeholder
    QUrl downloadUrl = m_resolvedUrls.value(name,
        QUrl("https://snap.stanford.edu/data/" + id + ".txt.gz"));

    bool downloaded = QFile::exists(downloadDir + "/" + name + ".txt")
                   || QFile::exists(downloadDir + "/" + id + ".txt");

    auto *itemName  = new QStandardItem(name);
    auto *itemNodes = new QStandardItem(formatNumber(nodes));
    auto *itemEdges = new QStandardItem(formatNumber(edges));
    auto *itemTris  = new QStandardItem(tris > 0 ? formatNumber(tris) : "–");
    auto *itemDl    = new QStandardItem(downloaded ? "✓" : "");

    itemName->setData(downloadUrl,       Qt::UserRole);
    itemName->setData((qlonglong)nodes,  Qt::UserRole + ColNodes);
    itemName->setData((qlonglong)edges,  Qt::UserRole + ColEdges);
    itemName->setData((qlonglong)tris,   Qt::UserRole + ColTriangles);
    itemName->setData(id,                Qt::UserRole + 10);  // store id for page URL

    itemNodes->setTextAlignment(Qt::AlignCenter);
    itemEdges->setTextAlignment(Qt::AlignCenter);
    itemTris->setTextAlignment(Qt::AlignCenter);
    itemDl->setTextAlignment(Qt::AlignCenter);
    itemName->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    if (downloaded) itemDl->setForeground(QColor("#1e8449"));

    for (auto* item : {itemName, itemNodes, itemEdges, itemTris, itemDl})
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);

    datasetModel->appendRow({itemName, itemNodes, itemEdges, itemTris, itemDl});
}

// ---------------------------------------------------------------------------
// Refresh button — fetch live SNAP index
// ---------------------------------------------------------------------------
void SnapBrowserWidget::onRefreshClicked() {
    if (!refreshButton || !downloadManager) return;
    refreshButton->setEnabled(false);
    m_refreshingIndex = true;
    emit logMessage("Fetching latest SNAP index...");
    downloadManager->startCatalogDownload(
        QUrl("https://snap.stanford.edu/data/index.html"),
        getIndexFilePath());
}

void SnapBrowserWidget::handleCatalogReady(bool success) {
    if (!m_refreshingIndex) return;  // ignore catalogDownloaded from subpage fetches
    m_refreshingIndex = false;
    if (refreshButton) refreshButton->setEnabled(true);

    if (success) {
        QFile file(getIndexFilePath());
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            parseSnapHtml(QString::fromUtf8(file.readAll()));
            file.close();
        }
    } else {
        emit logMessage("Failed to fetch SNAP index — showing bundled catalog");
    }
}

// ---------------------------------------------------------------------------
// Robust HTML parser — structure independent
//
// Strategy: scan for ANY link matching /data/DATASETID.html
// This works regardless of table structure, CSS, or layout changes.
// Stats are filled from bundled JSON — no table parsing needed.
// New datasets (not in bundle) get empty stats filled lazily on selection.
// ---------------------------------------------------------------------------
void SnapBrowserWidget::parseSnapHtml(const QString &html) {
    if (!datasetModel) return;
    if (html.size() < 1000) {
        emit logMessage("WARNING: SNAP index HTML too small — may be incomplete");
        return;
    }

    // Match any link to a SNAP dataset page
    // Pattern: href="/data/DATASETID.html" ... link text
    QRegularExpression re(
        "href=\"(?:https?://snap\\.stanford\\.edu)?/data/"
        "([a-zA-Z0-9_\\-.]+)\\.html\"[^>]*>([^<]{2,80})</a>",
        QRegularExpression::CaseInsensitiveOption);

    // Known non-dataset page IDs to skip
    static const QStringList kSkipIds = {
        "index", "links", "about", "citing", "new", "biosnap",
        "memetracker", "temporal-motifs"
    };

    QSet<QString> seen;
    QSet<QString> existingIds;

    // Collect IDs already in the model (from bundled load)
    for (int i = 0; i < datasetModel->rowCount(); ++i) {
        auto* item = datasetModel->item(i, ColName);
        if (item) existingIds.insert(item->text());
    }

    int newCount = 0;
    auto it = re.globalMatch(html);

    while (it.hasNext()) {
        auto m = it.next();
        QString id   = m.captured(1).toLower();
        QString name = m.captured(2).trimmed();

        // Clean link text — remove any residual HTML entities
        name.replace("&amp;", "&").replace("&lt;", "<").replace("&gt;", ">");
        name = name.simplified();

        if (id.isEmpty() || name.isEmpty()) continue;
        if (seen.contains(id)) continue;
        if (std::any_of(kSkipIds.begin(), kSkipIds.end(),
            [&](const QString& s){ return id.contains(s); })) continue;
        if (name.length() < 3) continue;

        seen.insert(id);

        // Already in model from bundled catalog — skip
        if (existingIds.contains(name)) continue;

        // New dataset not in bundle — add with empty stats
        // Stats will be filled lazily when user selects it
        const BundledDataset* b = findBundled(id);
        int64_t nodes = b ? b->nodes : 0;
        int64_t edges = b ? b->edges : 0;
        int64_t tris  = b ? b->triangles : 0;

        addDatasetRow(id, name.isEmpty() ? id : name, nodes, edges, tris);
        newCount++;
    }

    emit logMessage(QString("SNAP index: %1 total datasets (%2 new from live index)")
                    .arg(datasetModel->rowCount()).arg(newCount));
}

// ---------------------------------------------------------------------------
// Sort indicator
// ---------------------------------------------------------------------------
void SnapBrowserWidget::onSortIndicatorChanged(int logicalIndex, Qt::SortOrder order) {
    if (datasetView && datasetView->header())
        datasetView->header()->setSortIndicator(logicalIndex, order);
}

void SnapBrowserWidget::onSearchTextChanged(const QString &text) {
    if (proxyModel) proxyModel->setFilterFixedString(text);
}

// ---------------------------------------------------------------------------
// URL validation — remove rows whose download URL returns 404
// ---------------------------------------------------------------------------
void SnapBrowserWidget::handleUrlValidated(const QUrl &url, bool isValid) {
    if (!datasetModel || isValid) return;
    for (int i = 0; i < datasetModel->rowCount(); ++i) {
        auto* item = datasetModel->item(i, ColName);
        if (item && item->data(Qt::UserRole).toUrl() == url) {
            datasetModel->removeRow(i);
            break;
        }
    }
}

// ---------------------------------------------------------------------------
// Download status
// ---------------------------------------------------------------------------
void SnapBrowserWidget::updateRowDownloadStatus(int row) {
    if (!datasetModel || row < 0 || row >= datasetModel->rowCount()) return;

    QString downloadDir = QStandardPaths::writableLocation(
                              QStandardPaths::DownloadLocation);
    auto* nameItem = datasetModel->item(row, ColName);
    if (!nameItem) return;

    QUrl url = nameItem->data(Qt::UserRole).toUrl();
    QString fileName = QFileInfo(url.path()).baseName();
    bool downloaded = QFile::exists(downloadDir + "/" + fileName + ".txt")
                   || QFile::exists(downloadDir + "/" + fileName);

    auto* dlItem = datasetModel->item(row, ColDownloaded);
    if (dlItem) {
        dlItem->setText(downloaded ? "✓" : "");
        dlItem->setForeground(downloaded ? QColor("#1e8449") : QColor());
    }
}

// ---------------------------------------------------------------------------
// Selection — emit datasetSelected, lazily fetch missing stats
// ---------------------------------------------------------------------------
void SnapBrowserWidget::handleSelectionChanged(
    const QItemSelection &selected, const QItemSelection &)
{
    if (!datasetModel || !proxyModel || !downloadManager) return;
    if (selected.indexes().isEmpty()) return;

    QModelIndex proxyIndex = selected.indexes().first();
    if (!proxyIndex.isValid()) return;

    QModelIndex sourceIndex = proxyModel->mapToSource(proxyIndex);
    if (!sourceIndex.isValid()) return;

    int row = sourceIndex.row();
    if (row < 0 || row >= datasetModel->rowCount()) return;

    auto* nameItem = datasetModel->item(row, ColName);
    if (!nameItem) return;

    QString name         = nameItem->text();
    QUrl    downloadUrl  = nameItem->data(Qt::UserRole).toUrl();
    int64_t triangles    = nameItem->data(Qt::UserRole + ColTriangles).toLongLong();
    int64_t nodes        = nameItem->data(Qt::UserRole + ColNodes).toLongLong();
    int64_t edges        = nameItem->data(Qt::UserRole + ColEdges).toLongLong();

    updateRowDownloadStatus(row);

    // Apply cached triangles from persisted cache if not already in model
    if (triangles <= 0 && m_trianglesCache.contains(name)) {
        triangles = m_trianglesCache[name];
        nameItem->setData((qlonglong)triangles, Qt::UserRole + ColTriangles);
        auto* tItem = datasetModel->item(row, ColTriangles);
        if (tItem) tItem->setText(formatNumber(triangles));
    }

    emit datasetSelected(name, downloadUrl, triangles);

    // Always fetch subpage to resolve real download URL + fill missing stats
    // Skip only if URL already resolved and stats complete
    bool missingUrl   = !m_resolvedUrls.contains(name);
    bool missingStats = (triangles <= 0 || nodes <= 0 || edges <= 0);

    if ((missingUrl || missingStats) && !m_fetchingTriangles.contains(name)) {
        m_fetchingTriangles.insert(name);

        QString datasetId = nameItem->data(Qt::UserRole + 10).toString();  // stored id
        if (datasetId.isEmpty()) datasetId = name;  // fallback

        QUrl subPageUrl("https://snap.stanford.edu/data/" + datasetId + ".html");
        QString tempPath = QStandardPaths::writableLocation(
                               QStandardPaths::TempLocation) + "/" + name + ".html";

        int     currentRow   = row;
        QString currentName  = name;

        connect(downloadManager, &DownloadManager::catalogDownloaded, this,
            [this, currentName, currentRow, tempPath](bool success) {
                if (!success) {
                    m_fetchingTriangles.remove(currentName);
                    return;
                }

                QFile file(tempPath);
                if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    m_fetchingTriangles.remove(currentName);
                    return;
                }

                QString html = QString::fromUtf8(file.readAll());
                file.close();
                QFile::remove(tempPath);

                // Parse stats
                auto parseNum = [&](const QString& label) -> int64_t {
                    QRegularExpression re(
                        label + "[^0-9]*([\\d,]+)",
                        QRegularExpression::CaseInsensitiveOption);
                    auto m = re.match(html);
                    if (!m.hasMatch()) return 0;
                    QString s = m.captured(1);
                    s.remove(",");
                    return s.toLongLong();
                };

                int64_t tris  = parseNum("Triangles?");
                int64_t nodes = parseNum("Nodes");
                int64_t edges = parseNum("Edges");

                // Parse real download URL — first .txt.gz link on the page
                QUrl resolvedUrl;
                QRegularExpression urlRe("href=\"([^\"]+\\.txt\\.gz)\"",
                                         QRegularExpression::CaseInsensitiveOption);
                auto urlMatch = urlRe.match(html);
                if (urlMatch.hasMatch()) {
                    QString href = urlMatch.captured(1);
                    if (href.startsWith("http")) {
                        resolvedUrl = QUrl(href);
                    } else if (href.startsWith("../")) {
                        // ../data/bigdata/... means relative to snap.stanford.edu/data/
                        // so go up one level from /data/ → snap.stanford.edu/
                        resolvedUrl = QUrl("https://snap.stanford.edu/" + href.mid(3));
                    } else if (href.startsWith("/")) {
                        resolvedUrl = QUrl("https://snap.stanford.edu" + href);
                    } else {
                        resolvedUrl = QUrl("https://snap.stanford.edu/data/" + href);
                    }
                } else {
                    // No .txt.gz found — dataset may use unsupported format (zip/csv)
                    emit logMessage(QString("⚠ %1: no .txt.gz download found on SNAP page — dataset may not be supported").arg(currentName));
                }

                // Update model
                if (datasetModel && currentRow >= 0
                        && currentRow < datasetModel->rowCount()) {
                    auto* ni = datasetModel->item(currentRow, ColName);
                    if (ni && ni->text() == currentName) {
                        // Store resolved URL
                        if (resolvedUrl.isValid()) {
                            ni->setData(resolvedUrl, Qt::UserRole);
                            m_resolvedUrls[currentName] = resolvedUrl;
                            // Re-emit datasetSelected with correct URL if this is still selected
                            emit datasetSelected(currentName, resolvedUrl,
                                ni->data(Qt::UserRole + ColTriangles).toLongLong());
                        }
                        if (nodes > 0) {
                            ni->setData((qlonglong)nodes, Qt::UserRole + ColNodes);
                            auto* nItem = datasetModel->item(currentRow, ColNodes);
                            if (nItem) nItem->setText(formatNumber(nodes));
                        }
                        if (edges > 0) {
                            ni->setData((qlonglong)edges, Qt::UserRole + ColEdges);
                            auto* eItem = datasetModel->item(currentRow, ColEdges);
                            if (eItem) eItem->setText(formatNumber(edges));
                        }
                        if (tris > 0) {
                            ni->setData((qlonglong)tris, Qt::UserRole + ColTriangles);
                            auto* tItem = datasetModel->item(currentRow, ColTriangles);
                            if (tItem) tItem->setText(formatNumber(tris));
                            m_trianglesCache[currentName] = tris;
                            saveCache();
                        }
                        emit datasetMetadataLoaded(currentName, nodes, edges, tris);
                    }
                }

                m_fetchingTriangles.remove(currentName);
            }, Qt::SingleShotConnection);

        downloadManager->startCatalogDownload(subPageUrl, tempPath);
    } else if (!missingStats) {
        emit datasetMetadataLoaded(name, nodes, edges, triangles);
    }
}

// ---------------------------------------------------------------------------
// Double click — request download
// ---------------------------------------------------------------------------
void SnapBrowserWidget::onDoubleClicked(const QModelIndex &proxyIndex) {
    if (!datasetModel || !proxyModel || !proxyIndex.isValid()) return;

    QModelIndex sourceIndex = proxyModel->mapToSource(proxyIndex);
    if (!sourceIndex.isValid()) return;

    int row = sourceIndex.row();
    if (row < 0 || row >= datasetModel->rowCount()) return;

    auto* item = datasetModel->item(row, ColName);
    if (item)
        emit downloadRequested(item->text(), item->data(Qt::UserRole).toUrl());
}

// ---------------------------------------------------------------------------
// URL lookup
// ---------------------------------------------------------------------------
QUrl SnapBrowserWidget::getUrlForDataset(const QString& filename) const {
    if (!datasetModel) return QUrl();
    auto items = datasetModel->findItems(filename,
                     Qt::MatchExactly | Qt::MatchRecursive, 0);
    return items.isEmpty() ? QUrl() : items.first()->data(Qt::UserRole).toUrl();
}

// ---------------------------------------------------------------------------
// Triangle cache persistence
// ---------------------------------------------------------------------------
void SnapBrowserWidget::loadCache() {
    QFile file(cacheFilePath());
    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    for (const QJsonValue& val : root["triangles"].toArray()) {
        QJsonObject o = val.toObject();
        m_trianglesCache[o["name"].toString()] =
            o["triangles"].toVariant().toLongLong();
    }
}

void SnapBrowserWidget::saveCache() {
    QJsonArray arr;
    for (auto it = m_trianglesCache.constBegin();
         it != m_trianglesCache.constEnd(); ++it) {
        QJsonObject obj;
        obj["name"]      = it.key();
        obj["triangles"] = qint64(it.value());
        arr.append(obj);
    }
    QJsonObject root;
    root["triangles"] = arr;

    QFile file(cacheFilePath());
    QDir().mkpath(QFileInfo(file).absolutePath());
    if (file.open(QIODevice::WriteOnly))
        file.write(QJsonDocument(root).toJson());
}

QString SnapBrowserWidget::cacheFilePath() const {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + "/cache/snap_cache.json";
}

void SnapBrowserWidget::refreshDownloadedStatus() {
    for (int row = 0; row < datasetModel->rowCount(); ++row) {
        QStandardItem *item = datasetModel->item(row);
        if (!item) continue;

        QString originalName = item->data(Qt::UserRole).toString();
        if (originalName.isEmpty()) continue;

        QString dataDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) + "/GraphAnalyzer";
        QString filePath = dataDir + "/" + originalName + ".txt";
        bool isDownloaded = QFile::exists(filePath);

        QString displayName = isDownloaded ? "✓ " + originalName : originalName;
        item->setText(displayName);
    }
}

void SnapBrowserWidget::markDatasetDownloaded(const QString &name) {
    // Find the item by its stored name (Qt::UserRole)
    for (int row = 0; row < datasetModel->rowCount(); ++row) {
        QStandardItem *item = datasetModel->item(row);
        if (!item) continue;

        QString itemName = item->data(Qt::UserRole).toString();
        if (itemName == name) {
            // Update display with checkmark
            QString displayName = "✓ " + itemName;
            item->setText(displayName);
            break;
        }
    }
}