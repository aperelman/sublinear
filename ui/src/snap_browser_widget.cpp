#include "snap_browser_widget.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QFile>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QDir>
#include <QUrl>

QString getIndexFilePath() {
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/data";
    QDir().mkpath(dataDir);
    return dataDir + "/snap_index.html";
}

SnapBrowserWidget::SnapBrowserWidget(DownloadManager *mgr, QWidget *parent)
    : QWidget(parent), downloadManager(mgr) {
    auto *layout = new QVBoxLayout(this);
    datasetModel = new QStandardItemModel(this);
    datasetView = new QTreeView(this);
    datasetView->setModel(datasetModel);
    datasetView->setRootIsDecorated(false);
    datasetView->header()->setVisible(false);
    datasetView->setSelectionBehavior(QAbstractItemView::SelectRows);
    datasetView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    datasetView->setAlternatingRowColors(true);
    datasetView->header()->setSectionResizeMode(QHeaderView::Stretch);

    refreshButton = new QPushButton("Refresh SNAP Index", this);
    layout->addWidget(datasetView, 1);
    layout->addWidget(refreshButton);

    connect(refreshButton, &QPushButton::clicked, this, &SnapBrowserWidget::onRefreshClicked);
    connect(datasetView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &SnapBrowserWidget::handleSelectionChanged);
    connect(datasetView, &QTreeView::doubleClicked, this, &SnapBrowserWidget::onDoubleClicked);

    if (downloadManager) {
        connect(downloadManager, &DownloadManager::catalogDownloaded, this, &SnapBrowserWidget::handleCatalogReady);
    }
}

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

    QRegularExpression re("<tr.*?>\\s*<td.*?>\\s*<a href=\"(.*?)\">(.*?)</a>\\s*</td>\\s*<td.*?>(.*?)</td>\\s*<td.*?>(.*?)</td>");
    re.setPatternOptions(QRegularExpression::DotMatchesEverythingOption);

    QRegularExpressionMatchIterator i = re.globalMatch(html);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString path = match.captured(1).trimmed();
        QString name = match.captured(2).trimmed();
        QString nodes = match.captured(3).trimmed().replace(",", "");
        QString edges = match.captured(4).trimmed().replace(",", "");

        if (!name.isEmpty()) {
            auto *nameItem = new QStandardItem(name);

            nameItem->setData(nodes, Qt::UserRole + 1);
            nameItem->setData(edges, Qt::UserRole + 2);
            nameItem->setData(qlonglong(-1), Qt::UserRole + 3);  // unvisited sentinel

            QUrl downloadUrl("https://snap.stanford.edu/data/" + name + ".txt.gz");
            nameItem->setData(downloadUrl, Qt::UserRole);

            datasetModel->appendRow(nameItem);
        }
    }
}

void SnapBrowserWidget::handleSelectionChanged(const QItemSelection &selected, const QItemSelection &) {
    if (selected.indexes().isEmpty()) return;
    QModelIndex index = selected.indexes().first();
    QStandardItem* item = datasetModel->itemFromIndex(index);
    QString name = item->text();

    if (item->data(Qt::UserRole + 3).toLongLong() == -1) {
        Q_EMIT logMessage("Deep visiting " + name + " for full metadata...");
        QUrl subPageUrl("https://snap.stanford.edu/data/" + name + ".html");
        QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/" + name + ".html";

        connect(downloadManager, &DownloadManager::finished, this, [this, item, tempPath](const QString &path) {
            if (path == tempPath) {
                QFile file(path);
                if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    parseDeepPage(file.readAll(), item);
                    file.close();
                    QFile::remove(path);
                }
            }
        }, Qt::SingleShotConnection);

        downloadManager->startDownload(subPageUrl, tempPath);
    } else {
        Q_EMIT datasetMetadataLoaded(
            item->text(),
            item->data(Qt::UserRole + 1).toLongLong(),
            item->data(Qt::UserRole + 2).toLongLong(),
            item->data(Qt::UserRole + 3).toLongLong()
        );
    }
}

void SnapBrowserWidget::parseDeepPage(const QString &html, QStandardItem *item) {
    QRegularExpression triRegex("Triangles\\s*</td>\\s*<td.*?>(\\d[\\d,]*)</td>", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = triRegex.match(html);
    int64_t triangles = match.hasMatch() ? match.captured(1).replace(",", "").toLongLong() : 0;

    item->setData(qlonglong(triangles), Qt::UserRole + 3);  // unambiguous cast

    Q_EMIT datasetMetadataLoaded(
        item->text(),
        item->data(Qt::UserRole + 1).toLongLong(),
        item->data(Qt::UserRole + 2).toLongLong(),
        qlonglong(triangles)
    );
    Q_EMIT logMessage("Deep visit complete: " + item->text());
}

void SnapBrowserWidget::onDoubleClicked(const QModelIndex &index) {
    if (!index.isValid()) return;
    QStandardItem* item = datasetModel->itemFromIndex(index);
    QString name = item->text();
    QUrl url = item->data(Qt::UserRole).toUrl();

    if (url.isEmpty())
        url = QUrl("https://snap.stanford.edu/data/" + name + ".txt.gz");

    Q_EMIT downloadRequested(name, url);
}

QUrl SnapBrowserWidget::getUrlForDataset(const QString& filename) const {
    QList<QStandardItem*> items = datasetModel->findItems(filename,
                                    Qt::MatchExactly | Qt::MatchRecursive, 0);
    if (!items.isEmpty())
        return items.first()->data(Qt::UserRole).toUrl();
    return QUrl();
}
