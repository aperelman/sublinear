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
#include <QSet>
#include <QItemSelection>

class DownloadManager;

// ---------------------------------------------------------------------------
// Numeric sort proxy — sorts node/edge/triangle columns numerically
// ---------------------------------------------------------------------------
class NumericSortProxyModel : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit NumericSortProxyModel(QObject *parent = nullptr)
        : QSortFilterProxyModel(parent) {}

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override {
        if (left.column() >= 1 && left.column() <= 3) {
            bool okL, okR;
            qint64 l = left.data(Qt::UserRole + left.column()).toLongLong(&okL);
            qint64 r = right.data(Qt::UserRole + right.column()).toLongLong(&okR);
            if (okL && okR) return l < r;
        }
        return QSortFilterProxyModel::lessThan(left, right);
    }
};

// ---------------------------------------------------------------------------
// Bundled dataset entry (loaded from snap_datasets.json Qt resource)
// ---------------------------------------------------------------------------
struct BundledDataset {
    QString id;
    QString name;
    int64_t nodes     = 0;
    int64_t edges     = 0;
    int64_t triangles = 0;
    QString type;
};

class SnapBrowserWidget : public QWidget {
    Q_OBJECT

public:
    explicit SnapBrowserWidget(DownloadManager *mgr, QWidget *parent = nullptr);
    ~SnapBrowserWidget() override;
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
    void onSortIndicatorChanged(int logicalIndex, Qt::SortOrder order);

private:
    // Bundled JSON catalog
    void loadBundledCatalog();
    const BundledDataset* findBundled(const QString& id) const;

    // HTML parsing — robust link scanner
    void parseSnapHtml(const QString &html);

    // Row helpers
    void addDatasetRow(const QString& id, const QString& name,
                       int64_t nodes, int64_t edges, int64_t tris);
    void updateRowDownloadStatus(int row);

    // Triangle cache (per-session + persisted)
    void loadCache();
    void saveCache();
    QString cacheFilePath() const;

    static QString formatNumber(int64_t n);
    static QString getIndexFilePath();

    DownloadManager*       downloadManager;
    QStandardItemModel*    datasetModel;
    NumericSortProxyModel* proxyModel;
    QLineEdit*             searchEdit;
    QTreeView*             datasetView;
    QPushButton*           refreshButton;

    QMap<QString, int64_t>    m_trianglesCache;
    QSet<QString>             m_fetchingTriangles;
    QList<BundledDataset>     m_bundledCatalog;
};

#endif
