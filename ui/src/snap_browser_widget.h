#ifndef SNAP_BROWSER_WIDGET_H
#define SNAP_BROWSER_WIDGET_H

#include <QWidget>
#include <QTreeView>
#include <QPushButton>
#include <QStandardItemModel>
#include <QItemSelection>
#include <QUrl>
#include "download_manager.h" // Required to recognize the manager type

class SnapBrowserWidget : public QWidget {
    Q_OBJECT
public:
    // Constructor now takes the shared DownloadManager
    explicit SnapBrowserWidget(DownloadManager *mgr, QWidget *parent = nullptr);

    signals:
        void datasetMetadataLoaded(const QString &name, int64_t nodes, int64_t edges, int64_t triangles);
    void logMessage(const QString &message);
    void downloadRequested(const QString &name, const QUrl &url);

private slots:
    void onRefreshClicked();
    void handleCatalogReady(bool success); // Slot to consume catalogDownloaded(bool)
    void handleSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void onDoubleClicked(const QModelIndex &index);

private:
    void parseSnapHtml(const QString &html);

    QTreeView *datasetView;
    QStandardItemModel *datasetModel;
    QPushButton *refreshButton;
    DownloadManager *downloadManager; // Reference to the central manager
};

#endif