#include "snap_browser_widget.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QFile>
#include <QRegularExpression>

SnapBrowserWidget::SnapBrowserWidget(DownloadManager *mgr, QWidget *parent)
    : QWidget(parent), downloadManager(mgr) {

    QVBoxLayout *layout = new QVBoxLayout(this);

    // Model with only one visible column
    datasetModel = new QStandardItemModel(this);
    datasetModel->setHorizontalHeaderLabels({"Dataset Name"});

    datasetView = new QTreeView(this);
    datasetView->setModel(datasetModel);

    // Make the TreeView look like a simple list
    datasetView->setRootIsDecorated(false);      // No expand/collapse arrows
    datasetView->setIndentation(0);               // No left-side padding
    datasetView->header()->setVisible(false);     // Hide the "Dataset Name" header
    datasetView->setSelectionBehavior(QAbstractItemView::SelectRows);
    datasetView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    datasetView->setAlternatingRowColors(true);

    datasetView->header()->setSectionResizeMode(QHeaderView::Stretch);
    layout->addWidget(datasetView, 1);

    refreshButton = new QPushButton("Refresh SNAP Index", this);
    layout->addWidget(refreshButton);

    // Connections
    connect(refreshButton, &QPushButton::clicked, this, &SnapBrowserWidget::onRefreshClicked);
    connect(datasetView, &QTreeView::doubleClicked, this, &SnapBrowserWidget::onDoubleClicked);
    connect(datasetView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &SnapBrowserWidget::handleSelectionChanged);

    connect(downloadManager, &DownloadManager::catalogDownloaded,
            this, &SnapBrowserWidget::handleCatalogReady);
}

void SnapBrowserWidget::onRefreshClicked() {
    refreshButton->setEnabled(false);
    emit logMessage("Fetching latest SNAP index...");
    downloadManager->startCatalogDownload(
        QUrl("https://snap.stanford.edu/data/index.html"),
        "C:/BIU/data/snap_index.html"
    );
}

void SnapBrowserWidget::handleCatalogReady(bool success) {
    refreshButton->setEnabled(true);
    if (success) {
        QFile file("C:/BIU/data/snap_index.html");
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = file.readAll();
            file.close();
            parseSnapHtml(content);
        }
    } else {
        emit logMessage("Network error. Could not reach SNAP servers.");
    }
}

void SnapBrowserWidget::parseSnapHtml(const QString &html) {
    datasetModel->removeRows(0, datasetModel->rowCount());

    // Regex to capture Name, Nodes, and Edges
    QRegularExpression re("<tr.*?>\\s*<td.*?>\\s*<a href=\".*?\">(.*?)</a>\\s*</td>\\s*<td.*?>(.*?)</td>\\s*<td.*?>(.*?)</td>");
    re.setPatternOptions(QRegularExpression::DotMatchesEverythingOption);

    QRegularExpressionMatchIterator i = re.globalMatch(html);

    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString name = match.captured(1).trimmed();
        QString nodes = match.captured(2).trimmed().replace(",", "");
        QString edges = match.captured(3).trimmed().replace(",", "");

        if (!name.isEmpty()) {
            QStandardItem* nameItem = new QStandardItem(name);

            // Hide technical data in UserRoles so they don't show up in the list
            nameItem->setData(nodes, Qt::UserRole + 1);
            nameItem->setData(edges, Qt::UserRole + 2);

            datasetModel->appendRow(nameItem);
        }
    }
    emit logMessage("Index synchronized.");
}

void SnapBrowserWidget::onDoubleClicked(const QModelIndex &index) {
    if (!index.isValid()) return;
    QString name = datasetModel->item(index.row(), 0)->text();
    emit downloadRequested(name, QUrl("https://snap.stanford.edu/data/" + name + ".txt.gz"));
}

void SnapBrowserWidget::handleSelectionChanged(const QItemSelection &selected, const QItemSelection &) {
    if (selected.indexes().isEmpty()) return;

    QModelIndex index = selected.indexes().first();
    QStandardItem* item = datasetModel->itemFromIndex(index);

    // Broadcast the hidden data to the main window labels
    emit datasetMetadataLoaded(
        item->text(),
        item->data(Qt::UserRole + 1).toLongLong(),
        item->data(Qt::UserRole + 2).toLongLong(),
        0
    );
}