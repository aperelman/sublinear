#ifndef SNAP_BROWSER_WIDGET_H
#define SNAP_BROWSER_WIDGET_H

#include <QWidget>
#include <QTreeView>
#include <QPushButton>
#include <QStandardItemModel>
#include <QItemSelection>
#include <QUrl>
#include "download_manager.h"

class SnapBrowserWidget : public QWidget {
    Q_OBJECT
public:
    explicit SnapBrowserWidget(DownloadManager *mgr, QWidget *parent = nullptr);
    QUrl getUrlForDataset(const QString& filename)const;
    Q_SIGNALS: // Required by QT_NO_KEYWORDS
        void datasetMetadataLoaded(const QString &name, int64_t nodes, int64_t edges, int64_t triangles);
    void logMessage(const QString &message);
    void downloadRequested(const QString &name, const QUrl &url);

private Q_SLOTS: // Required by QT_NO_KEYWORDS
    void onRefreshClicked();
    void handleCatalogReady(bool success);
    void handleSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void onDoubleClicked(const QModelIndex &index);

private:
    void parseSnapHtml(const QString &html);
    void parseDeepPage(const QString &html, QStandardItem *item); // For deep visit caching

    QTreeView *datasetView;
    QStandardItemModel *datasetModel;
    QPushButton *refreshButton;
    DownloadManager *downloadManager;
};

#endif