#include "graph_list_widget.h"
#include <QHeaderView>
#include <QRegularExpression>
#include <QTableWidgetItem>

GraphListWidget::GraphListWidget(QWidget *parent) : QTableWidget(parent) {
    setColumnCount(1);
    setHorizontalHeaderLabels({"Dataset Name"});
    horizontalHeader()->setStretchLastSection(true);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    verticalHeader()->setVisible(false);

    m_internalManager = new QNetworkAccessManager(this);

    connect(m_internalManager, &QNetworkAccessManager::finished, this, &GraphListWidget::onMainPageDownloaded);
    connect(this, &QTableWidget::itemSelectionChanged, this, &GraphListWidget::onRowSelected);
}

void GraphListWidget::fetchNamesOnly() {
    QUrl url("https://snap.stanford.edu/data/index.html");
    m_internalManager->get(QNetworkRequest(url));
}

void GraphListWidget::onMainPageDownloaded(QNetworkReply* reply) {
    if (reply->error() != QNetworkReply::NoError) {
        reply->deleteLater();
        return;
    }

    QString html = reply->readAll();
    reply->deleteLater();
    setRowCount(0);

    QRegularExpression re("<a href=\"([^\"]+?\\.html)\">([^<]+)</a>");
    QRegularExpressionMatchIterator i = re.globalMatch(html);

    setUpdatesEnabled(false);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString urlPath = match.captured(1);
        QString name = match.captured(2);

        if (urlPath.contains("/") || urlPath.startsWith("index")) continue;

        int row = rowCount();
        insertRow(row);
        QTableWidgetItem* item = new QTableWidgetItem(name);
        item->setData(Qt::UserRole, urlPath);
        setItem(row, 0, item);
    }
    setUpdatesEnabled(true);
}

void GraphListWidget::onRowSelected() {
    QList<QTableWidgetItem*> selected = selectedItems();
    if (selected.isEmpty()) return;

    QTableWidgetItem* item = selected.first();
    Q_EMIT requestMetadata(item->text(), item->data(Qt::UserRole).toString());
}