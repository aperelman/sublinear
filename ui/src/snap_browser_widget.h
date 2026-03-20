#ifndef SNAP_BROWSER_WIDGET_H
#define SNAP_BROWSER_WIDGET_H

#include <QWidget>
#include <QTreeView>
#include <QLineEdit>
#include <QPushButton>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QUrl>
#include <QMap>
#include <QItemSelection>

class DownloadManager;

class SnapBrowserWidget : public QWidget {
    Q_OBJECT

public:
    explicit SnapBrowserWidget(DownloadManager *mgr, QWidget *parent = nullptr);
    ~SnapBrowserWidget() override = default;
    QUrl getUrlForDataset(const QString& filename) const;

signals:
    void datasetSelected(const QString &name, const QUrl &url, int64_t triangles);
    void downloadRequested(const QString &name, const QUrl &url);
    void logMessage(const QString &message);
    void datasetMetadataLoaded(const QString &name, int64_t nodes, int64_t edges, int64_t triangles);

private slots:
    void onSearchTextChanged(const QString &text);
    void handleSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void onDoubleClicked(const QModelIndex &proxyIndex);
    void onRefreshClicked();
    void handleCatalogReady(bool success);
    void handleUrlValidated(const QUrl &url, bool isValid);

private:
    void loadCache();
    void saveCache();
    QString cacheFilePath() const;
    void parseSnapHtml(const QString &html);
    void parseDeepPage(const QString &html, QStandardItem *item);

    DownloadManager* downloadManager;
    QStandardItemModel* datasetModel;
    QSortFilterProxyModel* proxyModel;
    QLineEdit* searchEdit;
    QTreeView* datasetView;
    QPushButton* refreshButton;
    QMap<QString, int64_t> m_trianglesCache;
};

#endif