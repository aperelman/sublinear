#pragma once
#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include "snap_catalog.h"
#include "download_manager.h"

class SnapBrowserWidget : public QWidget {
    Q_OBJECT
public:
    explicit SnapBrowserWidget(QWidget* parent = nullptr);
    void setDatasets(const QVector<SnapDataset>& datasets);
signals:
    void datasetReady(const QString& filePath);
private slots:
    void onDatasetSelected();
    void onDownloadClicked();
    void onDownloadProgress(qint64 r, qint64 t);
    void onDownloadFinished(const QString& f);
    void onDownloadError(const QString& e);
private:
    void setupUI();
    void updateList();
    QString getPath(const SnapDataset& ds);
    bool isDownloaded(const SnapDataset& ds);
    QListWidget* list;
    QLabel* info;
    QPushButton* btn;
    QProgressBar* progress;
    QVector<SnapDataset> datasets;
    DownloadManager* dlmgr;
    QString dlPath;
};
